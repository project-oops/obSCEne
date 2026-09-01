//! Find checks that call a platform symbol without testing its address first.
//!
//! # The rule this enforces
//!
//! Every platform declaration is weak, so a symbol the loader could not resolve has a null
//! address. The harness skips a check whose *table row* symbol is null, because jumping to
//! zero ends the run and loses every check behind it.
//!
//! That guard covers one symbol. A check whose body calls anything else is on its own, and
//! `CLAUDE.md` says so (D058) - a rule written after a null `scePthreadRwlockInit` took the
//! host build down mid-check.
//!
//! Writing the rule down did not enforce it. This does.
//!
//! # Why the `try` record makes it worse than a crash
//!
//! A check announces its table-row symbol before running. If a *different* symbol it calls
//! is the null one, the process dies and the last line of the report names a function that
//! was never reached. Announce-before-attempting is the property this program is arranged
//! around; an unguarded call does not merely risk a crash, it makes the report lie about
//! where the crash was.
//!
//! # Ported from Python, and the port paid for itself before it was finished
//!
//! This was `scripts/guards.py`, and the two faults found while porting it are the argument
//! for having done so. It required the symbol column to be an identifier, so it silently
//! skipped **eight real rows** - every check whose symbol is descriptive rather than a name.
//! It also assumed every runner is called `check_*`; one is not. Both omissions include
//! `910-bulk/probe`, which calls arbitrary censused symbols and is the check a guard rule
//! least wants to miss.
//!
//! Neither fault could ever have surfaced as a failure. They surfaced as a smaller number
//! that nobody was comparing against anything - which is this project's most-repeated
//! defect, and why the gates belong where the tests are.

use crate::sections;
use std::collections::BTreeSet;
use std::path::Path;

/// A check that calls a platform symbol it never tested the address of.
#[derive(Debug, PartialEq, Eq)]
pub struct Unguarded {
    /// The check's identifier, as it appears in the report.
    pub check_id: String,
    /// The symbol the check announces in its `try` record.
    pub declared: String,
    /// Symbols it calls without taking the address first.
    pub also_calls: Vec<String>,
}

/// Every name a section could call through the platform.
///
/// Read from `src/imports.c` rather than the headers: it is the file `mkmodule` validates
/// against, so it cannot be out of date without the build failing.
///
/// Rows are `{"library", "symbol"},`, so splitting a line on the quote character puts the
/// symbol in the fourth field. Written as an iterator walk rather than offset arithmetic
/// because the crate denies both indexing and bare arithmetic - a policy aimed at the ELF
/// writer, and one this parser is better for anyway.
#[must_use]
pub fn platform_symbols(imports_c: &str) -> BTreeSet<String> {
    let mut out = BTreeSet::new();
    for line in imports_c.lines() {
        if !line.contains('{') {
            continue;
        }
        let mut fields = line.split('"');
        let (Some(_), Some(_library), Some(_), Some(symbol)) =
            (fields.next(), fields.next(), fields.next(), fields.next())
        else {
            continue;
        };
        if !symbol.is_empty() && symbol.chars().all(|c| c.is_alphanumeric() || c == '_') {
            out.insert(symbol.to_owned());
        }
    }
    out
}

/// Comments removed, so a symbol *named* in prose is not read as a call.
///
/// Without this, every explanatory comment mentioning a function reads as an unguarded call
/// to it - and this codebase comments heavily, so that is not a rare false positive but the
/// common case.
#[must_use]
pub fn strip_comments(text: &str) -> String {
    #[derive(Clone, Copy)]
    enum Mode {
        Code,
        MaybeOpen,
        Block,
        BlockMaybeClose,
        Line,
    }
    let mut mode = Mode::Code;
    let mut out = String::with_capacity(text.len());
    for c in text.chars() {
        mode = match (mode, c) {
            (Mode::Code, '/') => Mode::MaybeOpen,
            (Mode::Code, _) => {
                out.push(c);
                Mode::Code
            }
            (Mode::MaybeOpen, '*') => {
                out.push(' ');
                Mode::Block
            }
            (Mode::MaybeOpen, '/') => {
                out.push(' ');
                Mode::Line
            }
            (Mode::MaybeOpen, _) => {
                // The slash was division, not a comment. Both characters are code.
                out.push('/');
                out.push(c);
                Mode::Code
            }
            (Mode::BlockMaybeClose, '/') => Mode::Code,
            (Mode::Block | Mode::BlockMaybeClose, '*') => Mode::BlockMaybeClose,
            (Mode::Block | Mode::BlockMaybeClose, _) => Mode::Block,
            (Mode::Line, '\n') => {
                out.push('\n');
                Mode::Code
            }
            (Mode::Line, _) => Mode::Line,
        };
    }
    out
}

/// Each `static obs_result name(void) { ... }` body in a section file, by function name.
///
/// The body ends at the first newline followed immediately by a closing brace, which is
/// what the file's own formatting guarantees.
fn check_bodies(text: &str) -> Vec<(String, String)> {
    let mut out = Vec::new();
    for chunk in text.split("static obs_result ").skip(1) {
        let Some((name, rest)) = chunk.split_once('(') else {
            continue;
        };
        // Only the no-argument form is a check runner.
        let Some(rest) = rest.strip_prefix("void)") else {
            continue;
        };
        let Some((_, body)) = rest.split_once('{') else {
            continue;
        };
        let Some((body, _)) = body.split_once("\n}") else {
            continue;
        };
        out.push((name.trim().to_owned(), body.to_owned()));
    }
    out
}

/// One lexical token: an identifier, or a single punctuation character.
///
/// Whitespace is dropped, which is what lets "called" and "guarded" be answered by looking
/// at a token's neighbours rather than by counting characters.
fn tokenize(code: &str) -> Vec<String> {
    let mut out = Vec::new();
    let mut word = String::new();
    for c in code.chars() {
        if c.is_alphanumeric() || c == '_' {
            word.push(c);
            continue;
        }
        if !word.is_empty() {
            out.push(std::mem::take(&mut word));
        }
        if !c.is_whitespace() {
            out.push(c.to_string());
        }
    }
    if !word.is_empty() {
        out.push(word);
    }
    out
}

/// Symbols the body calls, and symbols whose address it takes.
///
/// A call is a symbol followed by an opening parenthesis; a guard is a symbol preceded by
/// `&`, which is what every spelling of the weak-linkage test has in common - `&sym != 0`,
/// `OBS_REQUIRE(&sym)`, `obs_address_is_callable((const void *)&sym)`.
fn called_and_guarded(
    code: &str,
    symbols: &BTreeSet<String>,
) -> (BTreeSet<String>, BTreeSet<String>) {
    let mut called = BTreeSet::new();
    let mut guarded = BTreeSet::new();

    // Padded so every real token has a neighbour on each side, which removes the only place
    // this would otherwise need index arithmetic.
    let mut tokens = vec![String::new()];
    tokens.extend(tokenize(code));
    tokens.push(String::new());

    for window in tokens.windows(3) {
        let (Some(before), Some(word), Some(after)) =
            (window.first(), window.get(1), window.get(2))
        else {
            continue;
        };
        if !symbols.contains(word.as_str()) {
            continue;
        }
        if after == "(" {
            called.insert(word.clone());
        }
        if before == "&" {
            guarded.insert(word.clone());
        }
    }
    (called, guarded)
}

/// Every unguarded call across the section files, and how many checks were examined.
pub fn scan(sections_dir: &Path, imports_c: &str) -> std::io::Result<(Vec<Unguarded>, usize)> {
    let symbols = platform_symbols(imports_c);
    let mut problems = Vec::new();
    let mut total = 0usize;

    let mut paths: Vec<_> = std::fs::read_dir(sections_dir)?
        .filter_map(Result::ok)
        .map(|entry| entry.path())
        .filter(|p| p.extension().is_some_and(|e| e == "c"))
        .collect();
    paths.sort();

    for path in paths {
        let text = std::fs::read_to_string(&path)?;
        let bodies = check_bodies(&text);
        let mut rows = sections::rows_in(&text);
        rows.sort();
        for row in rows {
            let (check_id, declared, runner) = (row.id, row.symbol, row.runner);
            total = total.saturating_add(1);
            let Some((_, body)) = bodies.iter().find(|(name, _)| *name == runner) else {
                continue;
            };
            let code = strip_comments(body);
            let (called, guarded) = called_and_guarded(&code, &symbols);
            let also_calls: Vec<String> = called
                .into_iter()
                .filter(|s| !guarded.contains(s) && *s != declared)
                .collect();
            if !also_calls.is_empty() {
                problems.push(Unguarded {
                    check_id,
                    declared,
                    also_calls,
                });
            }
        }
    }
    Ok((problems, total))
}

#[cfg(test)]
mod tests {
    use super::*;

    fn symbols() -> BTreeSet<String> {
        platform_symbols(
            "{\"libkernel\", \"sceKernelOpen\"},\n{\"libkernel\", \"sceKernelClose\"},",
        )
    }

    #[test]
    fn reads_symbols_from_the_import_manifest() {
        let s = symbols();
        assert!(s.contains("sceKernelOpen"));
        assert!(s.contains("sceKernelClose"));
        assert!(!s.contains("libkernel"), "the library is not a symbol");
    }

    #[test]
    fn a_call_without_the_address_is_unguarded() {
        let (called, guarded) = called_and_guarded("int rc = sceKernelOpen(0);", &symbols());
        assert!(called.contains("sceKernelOpen"));
        assert!(guarded.is_empty());
    }

    #[test]
    fn taking_the_address_guards_it_however_it_is_spelled() {
        for code in [
            "if (&sceKernelOpen == 0) { return 0; } sceKernelOpen(0);",
            "OBS_REQUIRE(&sceKernelOpen); sceKernelOpen(0);",
            "obs_address_is_callable((const void *)&sceKernelOpen); sceKernelOpen(0);",
            "if (& sceKernelOpen != 0) { sceKernelOpen(0); }",
        ] {
            let (_, guarded) = called_and_guarded(code, &symbols());
            assert!(guarded.contains("sceKernelOpen"), "not guarded: {code}");
        }
    }

    /// The false positive that makes or breaks this check: a heavily commented codebase
    /// mentions functions in prose constantly, and every mention would read as a call.
    #[test]
    fn a_symbol_named_in_a_comment_is_not_a_call() {
        let code = strip_comments(
            "/* sceKernelOpen(0) would fail here. */\n// and sceKernelClose(0) too\nint x = 0;",
        );
        let (called, _) = called_and_guarded(&code, &symbols());
        assert!(
            called.is_empty(),
            "comments must not count as calls: {called:?}"
        );
    }

    /// Division is not a comment. A state machine that assumes otherwise eats the rest of
    /// the function.
    #[test]
    fn a_lone_slash_is_not_a_comment() {
        assert_eq!(strip_comments("int x = a / b;"), "int x = a / b;");
    }

    /// Fixture deliberately spelled the way the real tables are.
    ///
    /// The first version of this test used placeholders, and passed against a parser that
    /// guessed at row shape. It stopped passing the moment the parser started requiring the
    /// tokens a real row actually has - which is the test doing its job. A fixture that does
    /// not look like the thing under test proves nothing about the thing under test.
    #[test]
    fn the_announced_symbol_does_not_need_its_own_guard() {
        let text = "static obs_result check_thing(void) {\n    int rc = sceKernelOpen(0);\n    return rc;\n}\nstatic const obs_check t[] = {\n    {\"040-file/thing\", \"libkernel\", \"sceKernelOpen\", OBS_CAP_NONE, OBS_CAP_NONE,\n     (const void *)&sceKernelOpen, check_thing, OBS_FROM_SPEC},\n};";
        let bodies = check_bodies(text);
        assert_eq!(bodies.len(), 1);
        let rows = sections::rows_in(text);
        assert_eq!(rows.len(), 1);
        assert_eq!(
            rows.first().map(|r| r.symbol.as_str()),
            Some("sceKernelOpen")
        );
        assert_eq!(rows.first().map(|r| r.runner.as_str()), Some("check_thing"));
    }

    /// The Python original's blind spot: it required the symbol column to be an identifier,
    /// so eight real rows were never examined. Every string here is a real one.
    #[test]
    fn a_descriptive_symbol_is_still_a_row() {
        for symbol in [
            "(self-check)",
            "mapped memory",
            "(census control)",
            "(every censused symbol)",
            "(compute dispatch)",
        ] {
            let text = format!(
                "{{\"900-x/y\", \"lib\", \"{symbol}\", OBS_CAP_NONE, OBS_CAP_NONE, p, check_thing, OBS_FROM_ASSUMED}},"
            );
            let rows = sections::rows_in(&text);
            assert_eq!(rows.len(), 1, "row with symbol {symbol:?} was skipped");
            assert_eq!(rows.first().map(|r| r.symbol.as_str()), Some(symbol));
        }
    }

    /// A string that merely looks like a check id is not a row. `obs_report_progress` takes
    /// one as its first argument, and the first version of this port counted it.
    #[test]
    fn a_check_id_in_a_call_is_not_a_row() {
        let text = "obs_report_progress(\"910-bulk/probe\", (uint64_t)position);";
        assert!(sections::rows_in(text).is_empty());
    }

    /// The runner is positional. One row in the tree does not use the `check_` prefix, and
    /// it is the blind prober - the check this gate least wants to skip.
    #[test]
    fn the_runner_need_not_be_called_check_something() {
        let text = "{\"910-bulk/probe\", \"obscene\", \"(every censused symbol)\", OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&run_bulk, run_bulk, OBS_FROM_DERIVED},";
        let rows = sections::rows_in(text);
        assert_eq!(rows.len(), 1);
        assert_eq!(rows.first().map(|r| r.runner.as_str()), Some("run_bulk"));
    }

    #[test]
    fn a_second_symbol_in_the_body_is_reported() {
        let code = "sceKernelOpen(0); sceKernelClose(1);";
        let (called, guarded) = called_and_guarded(code, &symbols());
        let also: Vec<_> = called
            .into_iter()
            .filter(|s| !guarded.contains(s) && s != "sceKernelOpen")
            .collect();
        assert_eq!(also, vec!["sceKernelClose".to_string()]);
    }
}
