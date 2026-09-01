//! What the mined corpus was read from, and whether it has fallen behind.
//!
//! # The half of census drift nothing was watching
//!
//! `obscene-tool counts` gates the generated headers against `data/`. Nothing gated `data/`
//! against the emulators it was mined out of, and that is the drift with consequences: an
//! emulator gains four hundred names in a release, the corpus does not, and obSCEne reports
//! `absent` for a surface it never asked about. Every symptom points at the platform, and
//! the census reads as complete the entire time - because it is complete with respect to a
//! question asked months ago.
//!
//! # Recorded, not recomputed
//!
//! Re-mining takes minutes and needs both the emulator checkouts and the 23-version firmware
//! tree, so it cannot be a gate. Both data files carry a `mined-from:` line of
//! `<source>@<commit>` and a `firmware:` line of versions, and this compares those against
//! what is on disk.
//!
//! That is a weaker claim than "the names are current" and it is the claim that can actually
//! be made: *this corpus has not been shown the current commit*, never *these names changed*.
//! A new commit that touched no export table is a false positive and re-mining clears it in
//! minutes. The opposite error - a corpus quietly older than its sources - has no such
//! recovery, because nothing ever announces it.
//!
//! **Absent sources are not drift.** A machine without the checkouts says so and passes: it
//! has no evidence either way, and a gate that fails on missing evidence is one people learn
//! to skip.

use std::path::Path;

/// Checkouts the corpus is mined from. The emulator source directories, plus the two flat
/// catalogues' own repositories.
const SOURCES: [&str; 8] = [
    "ChonkyStation4",
    "GPCS4",
    "PS5PCEM",
    "SharpEMU",
    "craziiEmu",
    "fpPS4",
    "ps4_module_loader",
    "shadPS4",
];

/// The two header lines the corpus files carry, so `check` has something to compare.
pub fn provenance_lines(emulators: &Path, firmware: &Path) -> String {
    let (rows, versions) = provenance(emulators, firmware);
    let sources = if rows.is_empty() {
        "(no sources present)".to_owned()
    } else {
        rows.join(" ")
    };
    let firmware = if versions.is_empty() {
        "(none)".to_owned()
    } else {
        versions.join(",")
    };
    format!(
        "# mined-from: {sources}
# firmware: {firmware}
"
    )
}

/// `<source>@<commit>` for every checkout present, and the firmware versions on disk.
fn provenance(emulators: &Path, firmware: &Path) -> (Vec<String>, Vec<String>) {
    let mut rows = Vec::new();
    for name in SOURCES {
        let root = emulators.join(name);
        if !root.is_dir() {
            continue;
        }
        let mark = std::process::Command::new("git")
            .args(["-C"])
            .arg(&root)
            .args(["rev-parse", "--short", "HEAD"])
            .output()
            .ok()
            .filter(|o| o.status.success())
            .and_then(|o| String::from_utf8(o.stdout).ok())
            .map_or_else(|| "nogit".to_owned(), |s| s.trim().to_owned());
        let mark = if mark.is_empty() {
            "nogit".to_owned()
        } else {
            mark
        };
        rows.push(format!("{name}@{mark}"));
    }

    let mut versions: Vec<String> = std::fs::read_dir(firmware)
        .into_iter()
        .flatten()
        .filter_map(Result::ok)
        .map(|e| e.file_name().to_string_lossy().into_owned())
        .collect();
    versions.sort();
    (rows, versions)
}

/// Read a `# field: value` line out of the corpus header.
fn recorded(text: &str, field: &str) -> Option<String> {
    for line in text.lines() {
        if !line.starts_with('#') {
            break;
        }
        if let Some(rest) = line.strip_prefix(&format!("# {field}:")) {
            return Some(rest.trim().to_owned());
        }
    }
    None
}

/// Compare what the corpus says it read against what is on disk now.
pub fn check(root: &Path, emulators: &Path, firmware: &Path) -> std::io::Result<bool> {
    let path = root.join("data").join("mined-names.txt");
    if !path.is_file() {
        println!("no corpus at {} - nothing to check", path.display());
        return Ok(true);
    }
    let text = std::fs::read_to_string(&path)?;
    let Some(was_sources) = recorded(&text, "mined-from") else {
        println!("corpus predates provenance recording - re-run obscene-tool mine");
        return Ok(false);
    };
    let was_firmware = recorded(&text, "firmware").unwrap_or_default();

    let (rows, versions) = provenance(emulators, firmware);
    if rows.is_empty() {
        println!("corpus provenance: sources not present, not checked");
        return Ok(true);
    }
    let now_sources = rows.join(" ");
    let now_firmware = versions.join(",");
    if was_sources == now_sources && was_firmware == now_firmware {
        println!(
            "corpus provenance: current ({} sources, {} firmware versions)",
            rows.len(),
            versions.len()
        );
        return Ok(true);
    }

    println!("corpus is older than what it was mined from:");
    for (field, was, now) in [
        ("mined-from", was_sources.as_str(), now_sources.as_str()),
        ("firmware", was_firmware.as_str(), now_firmware.as_str()),
    ] {
        if was == now {
            continue;
        }
        let split = |s: &str| -> Vec<String> {
            s.replace(',', " ")
                .split_whitespace()
                .map(str::to_owned)
                .collect()
        };
        let (old, new) = (split(was), split(now));
        for item in &new {
            if !old.contains(item) {
                println!("  {field}: on disk, not in the corpus: {item}");
            }
        }
        for item in &old {
            if !new.contains(item) {
                println!("  {field}: in the corpus, not on disk:  {item}");
            }
        }
    }
    println!("  re-run obscene-tool mine");
    Ok(false)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn reads_a_recorded_field_from_the_header() {
        let text = "# a comment\n# mined-from: shadPS4@abc1234 fpPS4@def5678\n# firmware: 1.05,5.05\n#\nname lib id src fn\n";
        assert_eq!(
            recorded(text, "mined-from").as_deref(),
            Some("shadPS4@abc1234 fpPS4@def5678")
        );
        assert_eq!(recorded(text, "firmware").as_deref(), Some("1.05,5.05"));
    }

    /// The header ends at the first non-comment line; a `# mined-from:` appearing in the
    /// body would be data, not provenance.
    #[test]
    fn stops_at_the_end_of_the_header() {
        let text = "# firmware: 1.05\nname lib id src fn\n# mined-from: forged@0000000\n";
        assert_eq!(recorded(text, "mined-from"), None);
    }

    #[test]
    fn a_corpus_with_no_provenance_line_is_reported() {
        let text = "# Columns: name, library, identifiers, sources, kind.\n#\n";
        assert_eq!(recorded(text, "mined-from"), None);
    }
}
