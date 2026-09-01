//! The generated censuses: `corpus.h` (mined names) and `nids.h` (nameless identifiers).
//!
//! # Two headers, one input
//!
//! Both are generated from `data/` and both are committed, so nothing stops the data moving
//! while a header stays behind. A stale census reports an absence for a symbol the corpus no
//! longer claims, which reads as a platform gap rather than as a stale file - so both carry
//! a `--check` that `verify.sh` runs.
//!
//! # Names are declared as data, and that is the safety property
//!
//! Every censused name is `const char`, so the type system forbids calling it. A census of
//! forty thousand symbols is only safe to hold at all because none of them can be called by
//! accident; `910-bulk` steps around that deliberately, by casting an address rather than
//! redeclaring the name, which keeps the exception inside one expression.
//!
//! # Why the group tag carries an index
//!
//! Lowercasing a library name to make a C identifier collided `libkernel` with `libKernel` -
//! two real, distinct libraries the firmware ships - and produced two groups with one macro
//! name between them. The build caught it as a redefinition, which is the good case; the bad
//! one is two libraries silently sharing a table. An index makes the tag unique by
//! construction rather than by hoping names differ once case is thrown away.

use std::collections::{BTreeMap, BTreeSet};
use std::path::Path;

/// Names the compiler owns. Declaring one of these clashes with a builtin.
const COMPILER_OWNED: [&str; 3] = ["__atomic_", "__sync_", "__builtin_"];

/// Names that clash with a host libc global under `make host`.
const HOST_CLASH: [&str; 14] = [
    "errno", "environ", "h_errno", "timezone", "daylight", "tzname", "stdin", "stdout", "stderr",
    "optarg", "optind", "opterr", "optopt", "signgam",
];

/// A count with thousands separators, as the headers have always rendered them.
///
/// Cosmetic, and kept because these headers are committed: a formatting change would make
/// every regeneration a whole-file diff, which buries the one line that actually moved.
fn grouped(value: usize) -> String {
    let digits = value.to_string();
    let mut out = String::new();
    let leading = digits.len() % 3;
    for (index, c) in digits.chars().enumerate() {
        if index > 0 && index % 3 == leading {
            out.push(',');
        }
        out.push(c);
    }
    out
}

/// A library obSCEne would plausibly import from.
fn is_platform_library(name: &str) -> bool {
    name.starts_with("libSce") || name.starts_with("libkernel") || name.starts_with("libc")
}

fn is_c_identifier(name: &str) -> bool {
    let mut chars = name.chars();
    chars
        .next()
        .is_some_and(|c| c.is_ascii_alphabetic() || c == '_')
        && chars.all(|c| c.is_ascii_alphanumeric() || c == '_')
}

/// The eleven-character encoded form. Anything else in that column is a parse failure
/// upstream rather than an identifier.
fn is_encoded(text: &str) -> bool {
    text.len() == 11
        && text
            .chars()
            .all(|c| c.is_ascii_alphanumeric() || c == '+' || c == '_' || c == '-')
}

/// A unique C identifier for one library's group.
fn tag(order: &BTreeMap<String, usize>, library: &str) -> String {
    let index = order.get(library).copied().unwrap_or(0);
    let cleaned: String = library
        .chars()
        .map(|c| if c.is_ascii_alphanumeric() { c } else { '_' })
        .collect();
    format!("{index:04}_{cleaned}")
}

fn ordering(libraries: &BTreeSet<String>) -> BTreeMap<String, usize> {
    libraries
        .iter()
        .enumerate()
        .map(|(i, name)| (name.clone(), i))
        .collect()
}

/// Names already declared, which must not be declared again with a different type.
fn taken(surface: &str, platform: &str) -> BTreeSet<String> {
    let mut out = BTreeSet::new();
    for chunk in surface.split("X(").skip(1) {
        if let Some((inner, _)) = chunk.split_once(')') {
            let name = inner.trim();
            if is_c_identifier(name) && name != "name" {
                out.insert(name.to_owned());
            }
        }
    }
    // `OBS_WEAK int scePthreadMutexInit(...)` - the name immediately before the parenthesis.
    for chunk in platform.split("OBS_WEAK").skip(1) {
        let Some((head, _)) = chunk.split_once('(') else {
            continue;
        };
        let tail: String = head
            .trim_end()
            .chars()
            .rev()
            .take_while(|c| c.is_ascii_alphanumeric() || *c == '_')
            .collect();
        let name: String = tail.chars().rev().collect();
        if is_c_identifier(&name) {
            out.insert(name);
        }
    }
    out
}

/// Libraries `surface.h` already groups.
fn known_libraries(surface: &str) -> BTreeSet<String> {
    let mut out = BTreeSet::new();
    for chunk in surface.split("L(").skip(1) {
        let Some((_, rest)) = chunk.split_once(',') else {
            continue;
        };
        let Some(rest) = rest.trim_start().strip_prefix('"') else {
            continue;
        };
        if let Some((name, _)) = rest.split_once('"') {
            out.insert(name.to_owned());
        }
    }
    out
}

/// Which library set to emit.
#[derive(Clone, Copy, PartialEq, Eq)]
pub enum Scope {
    /// Only libraries `surface.h` already uses.
    Used,
    /// Those, plus `libSce*`, `libkernel*`, `libc*`.
    Platform,
    /// Every library with an attribution.
    All,
}

impl Scope {
    fn admits(self, library: &str, known: &BTreeSet<String>) -> bool {
        match self {
            Self::All => true,
            Self::Platform => known.contains(library) || is_platform_library(library),
            Self::Used => known.contains(library),
        }
    }
}

/// `X(name) \` per row, with the last carrying no continuation.
fn push_macro_rows(out: &mut Vec<String>, names: &[&String]) {
    let Some((last, rest)) = names.split_last() else {
        return;
    };
    for name in rest {
        out.push(format!("    X({name}) \\"));
    }
    out.push(format!("    X({last})"));
}

/// `L(tag, "library", OBS_SHARED, MACRO)` per group, joined with continuations.
fn group_rows(
    libraries: &BTreeSet<String>,
    order: &BTreeMap<String, usize>,
    macro_prefix: &str,
    tag_prefix: &str,
) -> String {
    libraries
        .iter()
        .map(|library| {
            let group = tag(order, library);
            format!(
                "    L({tag_prefix}{group}, \"{library}\", OBS_SHARED, {macro_prefix}{})",
                group.to_uppercase()
            )
        })
        .collect::<Vec<_>>()
        .join(" \\\n")
}

/// The fixed preamble of `corpus.h`, split out so the emitter stays readable.
fn corpus_header(total: usize) -> Vec<String> {
    vec![
        "/*".to_owned(),
        " * The mined census. Generated by obscene-tool census - do not edit.".to_owned(),
        " *".to_owned(),
        " * Every name here came from a published table: firmware module descriptions, or"
            .to_owned(),
        " * an emulator export list. See data/mined-names.txt for the sources per symbol"
            .to_owned(),
        " * and D105 for how the corpus was assembled.".to_owned(),
        " *".to_owned(),
        " * Declared exactly as surface.h declares its own: `const char`, so only the".to_owned(),
        " * address can ever be read. The type system forbidding the call is what makes a"
            .to_owned(),
        format!(
            " * census of {} symbols safe to hold at all.",
            grouped(total)
        ),
        " */".to_owned(),
        String::new(),
        "#ifndef OBSCENE_CORPUS_H".to_owned(),
        "#define OBSCENE_CORPUS_H".to_owned(),
        String::new(),
        "/* clang-format off */".to_owned(),
        String::new(),
        "/* On unless switched off. See the Makefile: a loader slow enough that the census"
            .to_owned(),
        " * dominates its runtime wants the curated one only, and that is a property of the"
            .to_owned(),
        " * loader rather than of this program. */".to_owned(),
        "#if !defined(OBS_NO_CORPUS)".to_owned(),
        String::new(),
    ]
}

/// Emit `corpus.h` from `data/mined-names.txt`.
fn render_corpus(root: &Path, scope: Scope) -> std::io::Result<String> {
    let include = root.join("include").join("obscene");
    let surface = std::fs::read_to_string(include.join("surface.h"))?;
    let platform = std::fs::read_to_string(include.join("platform.h"))?;
    let already = taken(&surface, &platform);
    let known = known_libraries(&surface);

    let data = std::fs::read_to_string(root.join("data").join("mined-names.txt"))?;
    let mut by_library: BTreeMap<String, Vec<(String, bool)>> = BTreeMap::new();
    let mut seen: BTreeSet<String> = BTreeSet::new();

    for line in data.lines() {
        if line.starts_with('#') {
            continue;
        }
        let parts: Vec<&str> = line.split_whitespace().collect();
        let (Some(name), Some(libraries)) = (parts.first(), parts.get(1)) else {
            continue;
        };
        // The fifth column, added when the blind prober died calling a data symbol. Absent
        // in an older corpus, so default to callable.
        let callable = parts.get(4).copied().unwrap_or("fn") != "data";
        if *libraries == "-" || already.contains(*name) || seen.contains(*name) {
            continue;
        }
        if !is_c_identifier(name)
            || COMPILER_OWNED.iter().any(|p| name.starts_with(p))
            || HOST_CLASH.contains(name)
        {
            continue;
        }
        // A name several libraries export goes to the first in scope, so it is declared
        // once. Which one is arbitrary: the identifier hashes the name alone, so any
        // exporting library resolves the same symbol.
        let Some(chosen) = libraries.split(',').find(|l| scope.admits(l, &known)) else {
            continue;
        };
        by_library
            .entry(chosen.to_owned())
            .or_default()
            .push(((*name).to_owned(), callable));
        seen.insert((*name).to_owned());
    }

    let libraries: BTreeSet<String> = by_library.keys().cloned().collect();
    let order = ordering(&libraries);
    let total = seen.len();

    let mut out = corpus_header(total);
    for (library, rows) in &by_library {
        let mut rows = rows.clone();
        rows.sort();
        let names: Vec<&String> = rows.iter().map(|(n, _)| n).collect();
        let callable: Vec<&String> = rows.iter().filter(|(_, c)| *c).map(|(n, _)| n).collect();
        let group = tag(&order, library).to_uppercase();
        out.push(format!(
            "/* {library}: {} symbols, {} callable. */",
            names.len(),
            callable.len()
        ));
        out.push(format!("#define OBS_CORPUS_{group}(X) \\"));
        push_macro_rows(&mut out, &names);

        // A second list holding only the functions.
        //
        // The census may name a data symbol, and counting one is fine. **Calling one jumps
        // into a variable**, which is how `910-bulk` died at `libSceImageUtil|__dso_handle`.
        // Two lists rather than a kind on every row, because the macro shape is shared with
        // hand-written `surface.h` and only one consumer reads the field.
        out.push(format!("#define OBS_CORPUS_CALLABLE_{group}(X) \\"));
        if callable.is_empty() {
            out.push("    /* none */".to_owned());
        } else {
            push_macro_rows(&mut out, &callable);
        }
        out.push(String::new());
    }

    out.push(
        "/* Every mined group, in the shape surface.h uses, so one expansion walks".to_owned(),
    );
    out.push(
        " * both. Availability is OBS_SHARED throughout: the corpus does not record".to_owned(),
    );
    out.push(
        " * which generation a symbol belongs to, and claiming otherwise would put".to_owned(),
    );
    out.push(" * a guess into the report. */".to_owned());
    out.push("#define OBS_CORPUS_LIBRARIES(L) \\".to_owned());
    out.push(group_rows(&libraries, &order, "OBS_CORPUS_", "corpus_"));
    out.push(String::new());
    out.push("#else /* OBS_NO_CORPUS */".to_owned());
    out.push(String::new());
    out.push("/* Empty, so every expansion site compiles unchanged. */".to_owned());
    out.push("#define OBS_CORPUS_LIBRARIES(L)".to_owned());
    out.push(String::new());
    out.push("#endif".to_owned());
    out.push(String::new());
    out.push(String::new());
    out.push("/* The same groups, functions only, for the blind prober. */".to_owned());
    out.push("#define OBS_CORPUS_CALLABLE_LIBRARIES(L) \\".to_owned());
    out.push(group_rows(
        &libraries,
        &order,
        "OBS_CORPUS_CALLABLE_",
        "corpus_",
    ));
    out.push(String::new());
    out.push("/* clang-format on */".to_owned());
    out.push(String::new());
    out.push("#endif /* OBSCENE_CORPUS_H */".to_owned());

    eprintln!(
        "symbols            : {} across {} libraries",
        grouped(total),
        by_library.len()
    );
    Ok(out.join("\n") + "\n")
}

/// Emit `nids.h` from `data/unnamed-nids.txt`.
fn render_nids(root: &Path, scope: Scope) -> std::io::Result<String> {
    let data = std::fs::read_to_string(root.join("data").join("unnamed-nids.txt"))?;
    let mut by_library: BTreeMap<String, BTreeSet<String>> = BTreeMap::new();
    let mut total = 0usize;

    for line in data.lines() {
        if line.starts_with('#') {
            continue;
        }
        let parts: Vec<&str> = line.split_whitespace().collect();
        let (Some(ident), Some(libraries)) = (parts.first(), parts.get(1)) else {
            continue;
        };
        if !is_encoded(ident) {
            continue;
        }
        let Some(library) = libraries.split(',').next() else {
            continue;
        };
        // An identifier can only be imported from the library that exports it - there is
        // nothing to recompute if the guess is wrong - so an entry without one is dropped.
        if library == "-" {
            continue;
        }
        let admitted = match scope {
            Scope::All => true,
            Scope::Platform => is_platform_library(library),
            Scope::Used => false,
        };
        if !admitted {
            continue;
        }
        by_library
            .entry(library.to_owned())
            .or_default()
            .insert((*ident).to_owned());
        total = total.saturating_add(1);
    }

    let libraries: BTreeSet<String> = by_library.keys().cloned().collect();
    let order = ordering(&libraries);

    let mut out = vec![
        "/*".to_owned(),
        " * The nameless census. Generated by obscene-tool census - do not edit.".to_owned(),
        " *".to_owned(),
        " * Identifiers observed in firmware export tables whose names are not recoverable."
            .to_owned(),
        " * Each is declared with a generated C name and an assembler label carrying the"
            .to_owned(),
        " * identifier itself, prefixed with `$` so mkmodule passes it through rather than"
            .to_owned(),
        " * hashing it. See D105 for where these came from.".to_owned(),
        " *".to_owned(),
        format!(
            " * {} identifiers across {} libraries.",
            grouped(total),
            by_library.len()
        ),
        " */".to_owned(),
        String::new(),
        "#ifndef OBSCENE_NIDS_H".to_owned(),
        "#define OBSCENE_NIDS_H".to_owned(),
        String::new(),
        "/* clang-format off */".to_owned(),
        String::new(),
        "#if defined(OBS_NIDS)".to_owned(),
        String::new(),
    ];

    let mut n = 0usize;
    for (library, idents) in &by_library {
        let group = tag(&order, library).to_uppercase();
        out.push(format!("/* {library}: {} identifiers. */", idents.len()));
        out.push(format!("#define OBS_NIDS_{group}(X) \\"));
        let mut rows = Vec::new();
        for ident in idents {
            n = n.saturating_add(1);
            rows.push(format!("    X(obs_nid_{n:07}, \"${ident}\")"));
        }
        out.push(rows.join(" \\\n"));
        out.push(String::new());
    }

    out.push(
        "/* Every nameless group. Availability is OBS_SHARED: the corpus records the".to_owned(),
    );
    out.push(
        " * library and the firmware versions, never the generation, and inventing one".to_owned(),
    );
    out.push(" * would put a guess where the report should carry a fact. */".to_owned());
    out.push("#define OBS_NID_LIBRARIES(L) \\".to_owned());
    out.push(group_rows(&libraries, &order, "OBS_NIDS_", "nid_"));
    out.push(String::new());
    out.push("#else /* !OBS_NIDS */".to_owned());
    out.push(String::new());
    out.push("/* Empty, so every expansion site compiles unchanged. A capability that".to_owned());
    out.push(" * removes its own macros when switched off makes the caller carry the".to_owned());
    out.push(" * condition, and the caller is the wrong place for it. */".to_owned());
    out.push("#define OBS_NID_LIBRARIES(L)".to_owned());
    out.push(String::new());
    out.push("#endif /* OBS_NIDS */".to_owned());
    out.push(String::new());
    out.push("/* clang-format on */".to_owned());
    out.push(String::new());
    out.push("#endif /* OBSCENE_NIDS_H */".to_owned());

    eprintln!(
        "identifiers        : {} across {} libraries",
        grouped(total),
        by_library.len()
    );
    Ok(out.join("\n") + "\n")
}

/// Which census to emit.
#[derive(Clone, Copy)]
pub enum Which {
    /// `include/obscene/corpus.h`, from the mined names.
    Corpus,
    /// `include/obscene/nids.h`, from the nameless identifiers.
    Nids,
}

/// Write or check one generated census. Returns true when it is current.
pub fn run(root: &Path, which: Which, scope: Scope, check: bool) -> std::io::Result<bool> {
    let (fresh, name) = match which {
        Which::Corpus => (render_corpus(root, scope)?, "corpus.h"),
        Which::Nids => (render_nids(root, scope)?, "nids.h"),
    };
    let path = root.join("include").join("obscene").join(name);
    if check {
        let current = std::fs::read_to_string(&path).unwrap_or_default();
        if current == fresh {
            println!("{name} current");
            return Ok(true);
        }
        println!("{name} is out of date - regenerate it");
        return Ok(false);
    }
    std::fs::write(&path, fresh)?;
    println!("written: {}", path.display());
    Ok(true)
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Lowercasing a library name collided `libkernel` with `libKernel` - two real,
    /// distinct libraries - and produced two groups with one macro name between them.
    #[test]
    fn group_tags_are_unique_across_case() {
        let libraries: BTreeSet<String> = ["libkernel".to_owned(), "libKernel".to_owned()]
            .into_iter()
            .collect();
        let order = ordering(&libraries);
        let a = tag(&order, "libkernel").to_uppercase();
        let b = tag(&order, "libKernel").to_uppercase();
        assert_ne!(a, b, "case-folded tags must not collide: {a} vs {b}");
    }

    #[test]
    fn only_real_c_identifiers_are_declarable() {
        assert!(is_c_identifier("sceKernelOpen"));
        assert!(is_c_identifier("_Z10getIpcPath"));
        assert!(!is_c_identifier("9lives"));
        assert!(!is_c_identifier("has space"));
        assert!(!is_c_identifier(""));
    }

    #[test]
    fn the_encoded_form_is_eleven_characters_of_the_alphabet() {
        assert!(is_encoded("Xh3kd9sLpQw"));
        assert!(is_encoded("4wSze92BhLI"));
        assert!(!is_encoded("tooshort"));
        assert!(!is_encoded("waaaaaaaaaaaytoolong"));
        assert!(!is_encoded("has!bad!char"));
    }

    #[test]
    fn platform_libraries_are_recognised() {
        assert!(is_platform_library("libSceNet"));
        assert!(is_platform_library("libkernel"));
        assert!(is_platform_library("libcInternal"));
        assert!(!is_platform_library("mscorlib"));
        assert!(!is_platform_library("libmonosgen"));
    }

    /// The last row of a macro carries no continuation, or the next line joins it.
    #[test]
    fn the_last_macro_row_has_no_continuation() {
        let a = "one".to_owned();
        let b = "two".to_owned();
        let mut out = Vec::new();
        push_macro_rows(&mut out, &[&a, &b]);
        assert_eq!(
            out,
            vec!["    X(one) \\".to_owned(), "    X(two)".to_owned()]
        );
    }

    #[test]
    fn counts_render_with_thousands_separators() {
        assert_eq!(grouped(0), "0");
        assert_eq!(grouped(958), "958");
        assert_eq!(grouped(34958), "34,958");
        assert_eq!(grouped(1_130_742), "1,130,742");
    }

    /// A name already declared in `platform.h` must not be censused as well: the census
    /// declares `const char` and the header declares a function, and the build fails.
    #[test]
    fn declared_names_are_excluded() {
        let already = taken(
            "X(sceKernelClose) \\\n",
            "OBS_WEAK int scePthreadMutexInit(ScePthreadMutex *m, const void *a);\n",
        );
        assert!(already.contains("sceKernelClose"));
        assert!(already.contains("scePthreadMutexInit"));
    }
}
