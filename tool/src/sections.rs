//! The check tables in `src/sections/*.c`, parsed once.
//!
//! # Why this is a module rather than a regex in each caller
//!
//! Three separate tools have hand-rolled a pattern for "a row in a check table", and all
//! three were wrong in different ways while reporting a confident number:
//!
//! - `guards.py` required the symbol column to be an identifier, so it skipped the eight
//!   rows whose symbol is descriptive - `"(self-check)"`, `"mapped memory"`,
//!   `"(census control)"`, `"(every censused symbol)"`. Among them `910-bulk/probe`, the
//!   check a guard rule least wants to miss.
//! - `counts.py` matched the section name as `[a-z]+`, so a section with a hyphen in it
//!   vanished. `150-memory-map` has two checks and the README said 134 instead of 136.
//! - The first Rust port of the guard check went the other way and counted string literals
//!   that merely look like check ids.
//!
//! Every one of those failed by producing a *smaller number*, which no gate can detect,
//! because a gate compares against nothing. The rot is not any single pattern - it is that
//! each tool grew its own.
//!
//! # What makes a row a row
//!
//! `{"id", "library", "symbol", OBS_CAP_…, OBS_CAP_…, address, runner, OBS_FROM_…},`
//!
//! The capability token is the anchor. No string literal elsewhere in the tree is followed
//! by one, so requiring it excludes prose and progress calls without excluding any real row.
//! The runner and the provenance are read **positionally** rather than by name: one runner in
//! the tree is not called `check_*`, and it is the blind prober.

use std::path::Path;

/// One row of a section's check table.
#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub struct Row {
    /// `NNN-section/name`, the identifier the report keys on.
    pub id: String,
    /// The library the announced symbol is imported from.
    pub library: String,
    /// The symbol announced in the `try` record. Not always an identifier - several are
    /// descriptive, and that is what the earlier parsers choked on.
    pub symbol: String,
    /// The function that runs the check.
    pub runner: String,
    /// The provenance token, lowercased and without its `OBS_FROM_` prefix.
    pub provenance: String,
    /// Capabilities the check needs before it may run, as written.
    pub requires: String,
    /// Capabilities a passing run of it grants, as written.
    pub provides: String,
}

/// Every row in one section file's tables.
#[must_use]
pub fn rows_in(text: &str) -> Vec<Row> {
    let mut out = Vec::new();
    for chunk in text.split('{').skip(1) {
        let Some(chunk) = chunk.strip_prefix('"') else {
            continue;
        };
        // id, separator, library, separator, symbol, and everything after.
        let mut parts = chunk.splitn(6, '"');
        let (Some(id), Some(gap1), Some(library), Some(gap2), Some(symbol), Some(rest)) = (
            parts.next(),
            parts.next(),
            parts.next(),
            parts.next(),
            parts.next(),
            parts.next(),
        ) else {
            continue;
        };
        let separates = |s: &str| s.chars().all(|c| c.is_whitespace() || c == ',');
        if !separates(gap1) || !separates(gap2) {
            continue;
        }
        if !rest
            .trim_start()
            .trim_start_matches(',')
            .trim_start()
            .starts_with("OBS_CAP")
        {
            continue;
        }
        let Some((row, _)) = rest.split_once("},") else {
            continue;
        };

        let fields: Vec<&str> = row.split(',').collect();
        // The first two capability fields, found rather than indexed.
        //
        // Whether the leading field is empty depends on where the previous split landed,
        // and a capability may be written as an or-of-two - which survives a comma split
        // intact but would break a fixed index. Searching for the token encodes neither
        // assumption.
        let mut caps = fields
            .iter()
            .map(|f| f.trim())
            .filter(|f| f.starts_with("OBS_CAP"));
        let (Some(requires), Some(provides)) = (caps.next(), caps.next()) else {
            continue;
        };
        // The runner is the field before the provenance. Position is the contract; a
        // `check_` naming convention is a habit, and one row breaks it.
        let found = fields.windows(2).find_map(|pair| {
            let (Some(before), Some(after)) = (pair.first(), pair.get(1)) else {
                return None;
            };
            let provenance = after.trim().strip_prefix("OBS_FROM_")?;
            let runner = before.trim();
            if runner.is_empty() || !runner.chars().all(|c| c.is_alphanumeric() || c == '_') {
                return None;
            }
            Some((runner.to_owned(), provenance.trim().to_lowercase()))
        });
        if let Some((runner, provenance)) = found {
            out.push(Row {
                id: id.to_owned(),
                library: library.to_owned(),
                symbol: symbol.to_owned(),
                runner,
                provenance,
                requires: requires.to_owned(),
                provides: provides.to_owned(),
            });
        }
    }
    out
}

/// Resolve a source sub-directory (supporting both src/probe/ and src/).
#[must_use]
pub fn find_dir(root: &Path, rel: &str) -> std::path::PathBuf {
    let probe_dir = root.join("src").join("probe").join(rel);
    if probe_dir.exists() {
        probe_dir
    } else {
        root.join("src").join(rel)
    }
}

/// Resolve a source file (supporting both src/probe/ and src/).
#[must_use]
pub fn find_file(root: &Path, filename: &str) -> std::path::PathBuf {
    let probe_file = root.join("src").join("probe").join(filename);
    if probe_file.exists() {
        probe_file
    } else {
        root.join("src").join(filename)
    }
}

/// Every row across every section file, sorted.
pub fn rows(sections_dir: &Path) -> std::io::Result<Vec<Row>> {
    let dir = if sections_dir.exists() {
        sections_dir.to_path_buf()
    } else if let Some(parent) = sections_dir.parent().and_then(|p| p.parent()) {
        find_dir(parent, "sections")
    } else {
        sections_dir.to_path_buf()
    };
    let mut paths: Vec<_> = std::fs::read_dir(&dir)?
        .filter_map(Result::ok)
        .map(|entry| entry.path())
        .filter(|p| p.extension().is_some_and(|e| e == "c"))
        .collect();
    paths.sort();

    let mut out = Vec::new();
    for path in paths {
        out.extend(rows_in(&std::fs::read_to_string(&path)?));
    }
    out.sort();
    Ok(out)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn row(id: &str, symbol: &str, runner: &str, provenance: &str) -> String {
        format!(
            "{{\"{id}\", \"libkernel\", \"{symbol}\", OBS_CAP_NONE, OBS_CAP_NONE, p, {runner}, OBS_FROM_{provenance}}},"
        )
    }

    #[test]
    fn reads_every_column() {
        let rows = rows_in(&row("040-file/open", "sceKernelOpen", "check_open", "SPEC"));
        assert_eq!(rows.len(), 1);
        let r = rows.first().expect("one row");
        assert_eq!(r.id, "040-file/open");
        assert_eq!(r.library, "libkernel");
        assert_eq!(r.symbol, "sceKernelOpen");
        assert_eq!(r.runner, "check_open");
        assert_eq!(r.provenance, "spec");
    }

    /// `counts.py` matched the section as `[a-z]+`, so a hyphen in it made the row vanish.
    /// `150-memory-map` has two checks, and the README claimed 134 instead of 136.
    #[test]
    fn a_hyphen_in_the_section_name_is_still_a_row() {
        for id in [
            "150-memory-map/walk",
            "150-memory-map/after-allocation",
            "007-responsive/libc",
        ] {
            let rows = rows_in(&row(id, "sym", "check_x", "ASSUMED"));
            assert_eq!(rows.len(), 1, "row {id} was skipped");
            assert_eq!(rows.first().map(|r| r.id.as_str()), Some(id));
        }
    }

    /// `guards.py` required the symbol column to be an identifier and skipped eight real
    /// rows. Every string here is one of them.
    #[test]
    fn a_descriptive_symbol_is_still_a_row() {
        for symbol in [
            "(self-check)",
            "mapped memory",
            "(census control)",
            "(every censused symbol)",
            "(compute dispatch)",
            "(qualifier)",
            "(symbol probe)",
        ] {
            let rows = rows_in(&row("900-x/y", symbol, "check_x", "ASSUMED"));
            assert_eq!(rows.len(), 1, "symbol {symbol:?} was skipped");
            assert_eq!(rows.first().map(|r| r.symbol.as_str()), Some(symbol));
        }
    }

    /// One runner in the tree is not called `check_*`, and it is the blind prober.
    #[test]
    fn the_runner_is_positional_not_prefixed() {
        let rows = rows_in(&row(
            "910-bulk/probe",
            "(every censused symbol)",
            "run_bulk",
            "DERIVED",
        ));
        assert_eq!(rows.first().map(|r| r.runner.as_str()), Some("run_bulk"));
    }

    /// A string that merely looks like a check id is not a row.
    #[test]
    fn a_check_id_in_a_call_is_not_a_row() {
        assert!(rows_in("obs_report_progress(\"910-bulk/probe\", (uint64_t)position);").is_empty());
        assert!(rows_in("static const char *id = \"040-file/open\";").is_empty());
    }
}
