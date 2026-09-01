//! Prose that describes behaviour, checked against the source it describes.
//!
//! # The failure this exists for
//!
//! `doccheck` catches prose that *names* something which does not exist - a file, a make rule,
//! a subcommand. It cannot catch prose that *describes behaviour*, because behaviour is not a
//! name and English is not diffable.
//!
//! `docs/PROTOCOL.md` spent part of a day stating "It binds loopback by default" after that
//! change had been reverted from `net_posix.c`, which still read `INADDR_ANY`. Nothing failed.
//! It was found by accident, while rewriting the section for an unrelated reason.
//!
//! That is the fourth instance this week of a mechanism reporting something reasonable while
//! being wrong, and the only one of the four with nothing to compare against (D158, D163 and
//! D168 were all caught by diffing two independent readings of the same fact).
//!
//! # What is checkable, and what is deliberately not
//!
//! Verifying a paragraph is not on the table. Verifying **one literal that the paragraph
//! depends on** is, and it is most of the value: the loopback claim rested entirely on which
//! constant `net_posix.c` passes to `htonl`.
//!
//! So a document may anchor a passage to a token in a source file:
//!
//! ```text
//! <!-- obscene:claim file=src/net_posix.c contains=INADDR_ANY -->
//! It binds every interface, because a console module must.
//! ```
//!
//! and `absent=` for the negative form. The gate says nothing about whether the prose is a
//! good description - only that the fact it stands on is still true.
//!
//! # Why this is opt-in, and why that is not a cop-out
//!
//! Every claim has to be written by hand, so coverage is whatever somebody bothered to
//! anchor. That is a real limit and it is the right trade: a gate that tried to infer claims
//! would produce false failures, and **a checker that cries wolf gets switched off** - which
//! `doccheck`'s own module documentation gives as the reason it matches only inside code
//! spans.
//!
//! The claims worth writing are the load-bearing ones: the security posture, the capability
//! list, the defaults. Those are also exactly the passages a reader acts on. (D170)

use std::collections::BTreeMap;
use std::path::Path;

/// One anchored claim.
pub struct Claim {
    /// The document making it, and the line the marker sits on.
    pub source: String,
    pub line: usize,
    /// The file it is a claim about.
    pub file: String,
    /// A token that must appear in that file.
    pub contains: Option<String>,
    /// A token that must not.
    pub absent: Option<String>,
}

/// Read the claims out of one document.
#[must_use]
pub fn parse(source: &str, text: &str) -> Vec<Claim> {
    let mut out = Vec::new();
    // Markers inside a fenced block are examples, not claims.
    //
    // `DECISIONS.md` shows the syntax in a fenced block while explaining it, and the first
    // version of this parser read that example as a live claim - five where four were
    // written. It happened to be true, which is the worst way for it to be wrong: a
    // documentation example that silently becomes a build dependency is a thing nobody
    // expects to break, and it would have broken the moment the example was edited to say
    // something illustrative rather than something current.
    //
    // Same reasoning as `doccheck` matching only inside code spans, arrived at from the
    // other direction.
    let mut fenced = false;
    for (index, line) in text.lines().enumerate() {
        if line.trim_start().starts_with("```") {
            fenced = !fenced;
            continue;
        }
        if fenced {
            continue;
        }
        let Some(rest) = line.trim().strip_prefix("<!-- obscene:claim ") else {
            continue;
        };
        let Some(body) = rest.strip_suffix("-->") else {
            continue;
        };
        let mut fields: BTreeMap<&str, &str> = BTreeMap::new();
        for pair in body.split_whitespace() {
            if let Some((key, value)) = pair.split_once('=') {
                fields.insert(key, value);
            }
        }
        let Some(file) = fields.get("file") else {
            continue;
        };
        out.push(Claim {
            source: source.to_owned(),
            // Humans count from one, and this number exists to be typed into an editor.
            line: index.saturating_add(1),
            file: (*file).to_owned(),
            contains: fields.get("contains").map(|s| (*s).to_owned()),
            absent: fields.get("absent").map(|s| (*s).to_owned()),
        });
    }
    out
}

/// Every claim in `docs/`, plus the top-level `CLAUDE.md` and `README.md`.
fn gather(root: &Path) -> std::io::Result<Vec<Claim>> {
    let mut out = Vec::new();
    let mut paths: Vec<std::path::PathBuf> = Vec::new();
    for name in ["CLAUDE.md", "README.md"] {
        let path = root.join(name);
        if path.exists() {
            paths.push(path);
        }
    }
    let docs = root.join("docs");
    if docs.is_dir() {
        let mut found: Vec<_> = std::fs::read_dir(&docs)?
            .filter_map(Result::ok)
            .map(|e| e.path())
            .filter(|p| p.extension().is_some_and(|e| e == "md"))
            .collect();
        found.sort();
        paths.extend(found);
    }
    for path in paths {
        let text = std::fs::read_to_string(&path)?;
        let name = path
            .strip_prefix(root)
            .unwrap_or(&path)
            .to_string_lossy()
            .replace('\\', "/");
        out.extend(parse(&name, &text));
    }
    Ok(out)
}

/// Run the gate.
pub fn run(root: &Path) -> std::io::Result<bool> {
    let claims = gather(root)?;
    if claims.is_empty() {
        println!("no anchored claims found");
        return Ok(true);
    }

    let mut broken = Vec::new();
    for claim in &claims {
        let path = root.join(&claim.file);
        let Ok(text) = std::fs::read_to_string(&path) else {
            broken.push(format!(
                "{}:{} claims about {}, which cannot be read",
                claim.source, claim.line, claim.file
            ));
            continue;
        };
        if let Some(token) = claim
            .contains
            .as_ref()
            .filter(|t| !text.contains(t.as_str()))
        {
            broken.push(format!(
                "{}:{} says {} contains {token}, and it does not",
                claim.source, claim.line, claim.file
            ));
        }
        if let Some(token) = claim.absent.as_ref().filter(|t| text.contains(t.as_str())) {
            broken.push(format!(
                "{}:{} says {} does not contain {token}, and it does",
                claim.source, claim.line, claim.file
            ));
        }
    }

    if broken.is_empty() {
        println!("claims: {} anchored passages still hold", claims.len());
        return Ok(true);
    }
    println!("{} anchored claim(s) no longer hold:", broken.len());
    for line in &broken {
        println!("  {line}");
    }
    println!("\nThe prose beside each marker describes behaviour the source no longer has.");
    println!("Fix the prose, or the code, or move the anchor - but not none of them. See D170.");
    Ok(false)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn a_marker_yields_a_claim() {
        let claims = parse(
            "docs/X.md",
            "prose\n<!-- obscene:claim file=src/net_posix.c contains=INADDR_ANY -->\nmore\n",
        );
        assert_eq!(claims.len(), 1);
        let c = claims.first().expect("one");
        assert_eq!(c.file, "src/net_posix.c");
        assert_eq!(c.contains.as_deref(), Some("INADDR_ANY"));
        assert_eq!(c.line, 2, "line numbers are for typing into an editor");
    }

    #[test]
    fn the_negative_form_parses() {
        let claims = parse(
            "d",
            "<!-- obscene:claim file=a.c absent=INADDR_LOOPBACK -->",
        );
        assert_eq!(
            claims.first().and_then(|c| c.absent.clone()).as_deref(),
            Some("INADDR_LOOPBACK")
        );
    }

    /// The decision log shows this syntax while explaining it. An example is not a claim.
    #[test]
    fn a_marker_inside_a_fence_is_an_example() {
        let text = "prose\n```text\n<!-- obscene:claim file=a.c contains=X -->\n```\nmore\n";
        assert!(parse("docs/DECISIONS.md", text).is_empty());
    }

    /// A marker with no `file=` is not a claim about anything, and is skipped rather than
    /// failing - an ordinary HTML comment must not be able to break the build.
    #[test]
    fn an_incomplete_marker_is_not_a_claim() {
        assert!(parse("d", "<!-- obscene:claim contains=X -->").is_empty());
        assert!(parse("d", "<!-- just a comment -->").is_empty());
    }
}
