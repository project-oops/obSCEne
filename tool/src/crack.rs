//! Recovering names from NIDs, by guessing.
//!
//! # Why guessing is the only route
//!
//! A NID is the first eight bytes of `SHA-1(name ‖ suffix)`. Hashing is one-way: given
//! `+g+UP8Pyfmo` there is no computation that yields `sceKernelGetProcessType`. The
//! reverse is trivial and this crate already does it.
//!
//! So the only way backwards is to hash candidates and look for a match. That is all this
//! module is - the hard part is having good candidates, and that lives outside it.
//!
//! # Not to be confused with `decode`
//!
//! `decode` undoes the *encoding*: the eleven printable characters back into the eight
//! bytes they stand for. Both are reversible and neither touches the hash. After decoding
//! you have the hash, and you are exactly as stuck.
//!
//! # Why this is tractable, and not password cracking
//!
//! The names follow a grammar rather than being arbitrary: a library prefix, a verb, a
//! noun. A few million candidates covers a great deal of it, which is seconds of hashing.
//! The work is in describing the convention, not in searching a keyspace.
//!
//! # Hits are proof, misses are nothing
//!
//! A match is certain - the hash agrees. A non-match says only that the candidate list
//! did not contain the name, never that no such name exists. Those must not be presented
//! alike, and the summary here says how many were tried precisely so a reader can see
//! what a miss is worth.
//!
//! # A table that has gone stale should say so
//!
//! The output records the suffix it used, the size of the candidate list, and how many
//! known pairs it reproduced. A generated table gets trusted long after it stops being
//! right, and the fix is the same one used for check provenance: make it carry its own
//! account of where it came from.

use std::collections::{BTreeMap, BTreeSet};

/// What a run recovered, and what it was worth.
#[derive(Debug, Default)]
pub struct Cracked {
    /// Recovered pairs, encoded NID to name, in NID order.
    pub found: BTreeMap<String, String>,
    /// Targets no candidate matched. Not evidence of anything about the names.
    pub unmatched: Vec<String>,
    /// How many candidates were hashed.
    pub tried: usize,
    /// Known pairs the candidate list reproduced, and how many were offered.
    ///
    /// The measure of the *generator*, not of the platform: a list that cannot
    /// regenerate names already known is not ready to be believed about unknown ones.
    pub known_hit: usize,
    pub known_total: usize,
}

impl Cracked {
    /// Whether the candidate list reproduced every known pair.
    ///
    /// The bar to clear before trusting a run against unknown NIDs. Short of it, a miss
    /// is at least as likely to be a gap in the candidates as a name that does not exist.
    #[must_use]
    pub fn generator_is_credible(&self) -> bool {
        self.known_total > 0 && self.known_hit == self.known_total
    }
}

/// Strips the `#library#module` suffix a module's symbol table carries.
///
/// Encoded names appear both ways: bare in a NID list, and decorated in a symbol table.
/// Accepting both means a caller can point this at either without a preparation step,
/// and a decorated NID silently failing to match would look like a name that does not
/// exist.
#[must_use]
pub fn bare_nid(text: &str) -> &str {
    match text.split_once('#') {
        Some((head, _)) => head,
        None => text,
    }
}

/// Hashes every candidate and matches against the targets.
///
/// `targets` are encoded NIDs, decorated or not. `candidates` are names to try. `known`
/// are pairs already established, used only to measure the candidate list.
#[must_use]
pub fn crack<'a>(
    targets: impl IntoIterator<Item = &'a str>,
    candidates: impl IntoIterator<Item = &'a str>,
    known: &[(String, String)],
    suffix: &[u8],
) -> Cracked {
    let mut wanted: BTreeSet<String> = BTreeSet::new();
    for target in targets {
        let bare = bare_nid(target.trim());
        if !bare.is_empty() {
            wanted.insert(bare.to_owned());
        }
    }

    let mut out = Cracked {
        known_total: known.len(),
        ..Cracked::default()
    };

    // One pass over the candidates, hashing each once and checking it against both the
    // targets and the known pairs. Two passes would double the only expensive part.
    let mut known_names: BTreeMap<&str, &str> = BTreeMap::new();
    for (encoded, name) in known {
        known_names.insert(name.as_str(), encoded.as_str());
    }

    for candidate in candidates {
        let name = candidate.trim();
        if name.is_empty() {
            continue;
        }
        out.tried = out.tried.saturating_add(1);
        let encoded = selfish_nid::Nid::with_suffix(name, suffix).encode();

        if wanted.contains(&encoded) {
            // First name wins. A second name hashing to the same NID is a collision in
            // eight bytes; it is possible, and taking the first keeps the result stable
            // rather than dependent on candidate order.
            out.found
                .entry(encoded.clone())
                .or_insert_with(|| name.to_owned());
        }
        if known_names
            .get(name)
            .is_some_and(|expected| *expected == encoded)
        {
            out.known_hit = out.known_hit.saturating_add(1);
        }
    }

    out.unmatched = wanted
        .iter()
        .filter(|encoded| !out.found.contains_key(*encoded))
        .cloned()
        .collect();
    out
}

/// Parses `nid name` lines, ignoring blanks and `#` comments.
///
/// The same shape the harvested corpus already has, so a file of known pairs can be fed
/// straight in without a conversion step.
#[must_use]
pub fn parse_pairs(text: &str) -> Vec<(String, String)> {
    let mut out = Vec::new();
    for line in text.lines() {
        let line = line.trim();
        if line.is_empty() || line.starts_with('#') {
            continue;
        }
        let mut parts = line.split_whitespace();
        if let (Some(encoded), Some(name)) = (parts.next(), parts.next()) {
            out.push((bare_nid(encoded).to_owned(), name.to_owned()));
        }
    }
    out
}

#[cfg(test)]
#[allow(
    clippy::indexing_slicing,
    clippy::arithmetic_side_effects,
    reason = "test fixtures build known-size buffers; a panic here is the failure \
              signal, which is the opposite of what these lints guard in the tool"
)]
mod tests {
    use super::{Cracked, bare_nid, crack, parse_pairs};

    /// The committed suffix, kept here rather than read from disk so the test pins the
    /// behaviour even if the data file is missing.
    const SUFFIX: [u8; 16] = [
        0x51, 0x8D, 0x64, 0xA6, 0x35, 0xDE, 0xD8, 0xC1, 0xE6, 0xB0, 0x39, 0xB1, 0xC3, 0xE5, 0x52,
        0x30,
    ];

    /// The published pair the whole chain rests on.
    const KNOWN_NID: &str = "wzvqT4UqKX8";
    const KNOWN_NAME: &str = "sceKernelLoadStartModule";

    #[test]
    fn a_candidate_that_hashes_to_the_target_is_recovered() {
        let out = crack([KNOWN_NID], [KNOWN_NAME, "sceSomethingElse"], &[], &SUFFIX);
        assert_eq!(
            out.found.get(KNOWN_NID).map(String::as_str),
            Some(KNOWN_NAME)
        );
        assert!(out.unmatched.is_empty());
    }

    #[test]
    fn a_target_nobody_guessed_is_reported_as_unmatched_not_as_absent() {
        // The distinction the whole module rests on: this says the candidate list was
        // short, not that the name does not exist.
        let out = crack([KNOWN_NID], ["sceNotThatOne"], &[], &SUFFIX);
        assert!(out.found.is_empty());
        assert_eq!(out.unmatched, vec![KNOWN_NID.to_owned()]);
        assert_eq!(out.tried, 1);
    }

    #[test]
    fn a_decorated_nid_matches_the_same_as_a_bare_one() {
        // Symbol tables carry `<nid>#lib#mod`. Failing to match one of those would look
        // exactly like a name that does not exist.
        let decorated = format!("{KNOWN_NID}#A#A");
        let out = crack([decorated.as_str()], [KNOWN_NAME], &[], &SUFFIX);
        assert_eq!(
            out.found.get(KNOWN_NID).map(String::as_str),
            Some(KNOWN_NAME)
        );
    }

    #[test]
    fn a_generator_is_measured_against_pairs_already_known() {
        let known = vec![(KNOWN_NID.to_owned(), KNOWN_NAME.to_owned())];
        let good = crack([], [KNOWN_NAME], &known, &SUFFIX);
        assert!(good.generator_is_credible());

        let bad = crack([], ["sceSomethingElse"], &known, &SUFFIX);
        assert!(!bad.generator_is_credible());
        assert_eq!(bad.known_hit, 0);
    }

    #[test]
    fn a_run_with_nothing_known_is_never_called_credible() {
        // Silence is not a pass. Without known pairs there is no evidence the candidate
        // list is any good, and saying so is the point of the field.
        let out = crack([KNOWN_NID], [KNOWN_NAME], &[], &SUFFIX);
        assert!(!out.generator_is_credible());
    }

    #[test]
    fn pairs_parse_from_the_shape_the_corpus_already_has() {
        let text = format!("# a comment\n{KNOWN_NID} {KNOWN_NAME}\n\n");
        let pairs = parse_pairs(&text);
        assert_eq!(pairs.len(), 1);
        assert_eq!(pairs[0].0, KNOWN_NID);
        assert_eq!(pairs[0].1, KNOWN_NAME);
    }

    #[test]
    fn the_suffix_is_stripped_from_a_decorated_nid() {
        assert_eq!(bare_nid("abc#B#C"), "abc");
        assert_eq!(bare_nid("abc"), "abc");
    }

    #[test]
    fn an_empty_run_claims_nothing() {
        let out = Cracked::default();
        assert!(!out.generator_is_credible());
        assert_eq!(out.tried, 0);
    }
}
