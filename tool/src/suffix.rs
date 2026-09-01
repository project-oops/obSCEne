//! Reading the NID hash suffix.
//!
//! **The constant itself lives in `selfish`**, compiled in, with its derivation and its
//! verification written beside it. This module is the *override*: a caller may point at a
//! different suffix file, which is what makes it possible to test a candidate against a
//! corpus without editing a committed constant.
//!
//! A local copy of the value would be a second place for it to be right, and the whole
//! argument for the sibling repository is that a constant with two homes eventually has two
//! values.
//!
//! An override file is parsed by hand rather than with a TOML crate. It has exactly one key,
//! and a dependency that exists to read one hex string is a dependency that will be used for
//! something else later without anyone deciding to.

use std::path::{Path, PathBuf};

/// The key carrying the value.
const KEY: &str = "suffix_hex";

/// Reads the suffix, defaulting to the committed one.
///
/// Accepts either the TOML data file or a raw binary file, so an override can be
/// whichever is to hand.
pub fn read(explicit: Option<&Path>) -> Result<Vec<u8>, SuffixError> {
    // No override means the committed constant, and the committed constant is `selfish`'s.
    let Some(given) = explicit else {
        return Ok(selfish_nid::suffix());
    };
    let path = given.to_path_buf();
    let raw = std::fs::read(&path).map_err(|source| SuffixError::Unreadable {
        path: path.clone(),
        source,
    })?;

    if path
        .extension()
        .is_some_and(|extension| extension == "toml")
    {
        let text = String::from_utf8_lossy(&raw);
        return parse_toml(&text).ok_or(SuffixError::NoKey { path });
    }
    Ok(raw)
}

/// Extracts `suffix_hex = "..."` from the data file.
fn parse_toml(text: &str) -> Option<Vec<u8>> {
    for line in text.lines() {
        let trimmed = line.trim_start();
        if trimmed.starts_with('#') {
            continue;
        }
        let Some(rest) = trimmed.strip_prefix(KEY) else {
            continue;
        };
        let Some(rest) = rest.trim_start().strip_prefix('=') else {
            continue;
        };
        let value = rest.trim().trim_matches('"');
        return decode_hex(value);
    }
    None
}

/// Decodes a hex string, refusing anything malformed rather than skipping bytes.
fn decode_hex(text: &str) -> Option<Vec<u8>> {
    if text.is_empty() || !text.len().is_multiple_of(2) {
        return None;
    }
    let bytes = text.as_bytes();
    let mut out = Vec::with_capacity(text.len() / 2);
    let mut index = 0;
    while index < bytes.len() {
        let pair = std::str::from_utf8(bytes.get(index..index.saturating_add(2))?).ok()?;
        out.push(u8::from_str_radix(pair, 16).ok()?);
        index = index.saturating_add(2);
    }
    Some(out)
}

/// Why the suffix could not be read.
#[derive(Debug)]
pub enum SuffixError {
    /// The file is missing or unreadable.
    Unreadable {
        /// Where it was looked for.
        path: PathBuf,
        /// What the filesystem said.
        source: std::io::Error,
    },
    /// The file exists but carries no usable value.
    NoKey {
        /// The file that was read.
        path: PathBuf,
    },
}

impl std::fmt::Display for SuffixError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Unreadable { path, source } => {
                write!(
                    f,
                    "cannot read the suffix from {}: {source}",
                    path.display()
                )
            }
            Self::NoKey { path } => write!(f, "{} has no usable `{KEY}` entry", path.display()),
        }
    }
}

impl std::error::Error for SuffixError {}

#[cfg(test)]
#[allow(
    clippy::indexing_slicing,
    clippy::arithmetic_side_effects,
    clippy::cast_possible_truncation,
    reason = "test fixtures build known-size buffers; a panic here is the failure               signal, which is the opposite of what these lints guard in the tool"
)]
mod tests {
    use super::{decode_hex, parse_toml};

    #[test]
    fn the_value_is_read_past_the_commentary() {
        // The real file is mostly explanation, so the parser has to skip a great deal
        // of prose - including lines that mention the key inside a comment.
        let text = "# suffix_hex is explained here\n# more prose\n\nsuffix_hex = \"00ff10\"\n";
        assert_eq!(parse_toml(text), Some(vec![0x00, 0xFF, 0x10]));
    }

    #[test]
    fn a_missing_key_is_reported_rather_than_defaulted() {
        assert_eq!(parse_toml("# nothing here\n"), None);
    }

    #[test]
    fn malformed_hex_is_refused_rather_than_partially_decoded() {
        // Silently dropping a bad byte would produce a shorter suffix that hashes to
        // plausible values and matches nothing.
        assert_eq!(decode_hex("0f0"), None, "odd length");
        assert_eq!(decode_hex("zz"), None, "not hex");
        assert_eq!(decode_hex(""), None, "empty");
        assert_eq!(decode_hex("00FF"), Some(vec![0x00, 0xFF]));
    }
}
