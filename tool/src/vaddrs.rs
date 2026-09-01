//! Resolve a vendor module's exports to `name vaddr`.
//!
//! # Why a line here is two facts joined, not one
//!
//! A module's export table stores each symbol by its NID - a hash of the name - and its
//! offset within the module. The offset is a measurement of the file; the name is not in the
//! file at all. So a `name vaddr` line is a join: the vaddr measured here, and the name looked
//! up in the mined corpus by the NID both sides carry.
//!
//! The corpus (`data/mined-names.txt`) records `name … <raw-nid>` per symbol. This decodes each
//! export's encoded NID to that same raw value and looks the name up. An export whose NID the
//! corpus does not know keeps its encoded NID as the label: no vaddr is dropped, and a bare hash
//! is the honest statement that the name is not yet known - which is exactly what a consumer's
//! own naming workflow then resolves.
//!
//! The vaddr is `measured`; the name is whatever the corpus row's sources make it. The header
//! records the module the vaddrs came from, so the two provenances stay separable.

use std::collections::BTreeMap;
use std::path::Path;

/// A raw-NID (`0x…`, lower-case, sixteen hex digits) to name map, from the corpus text.
///
/// Split from the file read so it can be tested without one. First name wins for a NID, which is
/// deterministic because the corpus is a committed file in a fixed order. A NID naming two
/// different symbols would be a hash collision, which the 64-bit space makes vanishingly unlikely
/// across one library's exports.
fn parse_corpus(text: &str) -> BTreeMap<String, String> {
    let mut names = BTreeMap::new();
    for line in text.lines() {
        if line.starts_with('#') {
            continue;
        }
        // Columns: name, libraries, identifiers, sources, kind. The identifiers are the NIDs.
        let parts: Vec<&str> = line.split_whitespace().collect();
        let (Some(name), Some(ids)) = (parts.first(), parts.get(2)) else {
            continue;
        };
        for id in ids.split(',') {
            if id == "-" || id.is_empty() {
                continue;
            }
            names
                .entry(id.to_ascii_lowercase())
                .or_insert_with(|| (*name).to_owned());
        }
    }
    names
}

/// Read the mined corpus and index it by NID.
fn corpus_names(corpus: &Path) -> std::io::Result<BTreeMap<String, String>> {
    Ok(parse_corpus(&std::fs::read_to_string(corpus)?))
}

/// The bytes to parse as an ELF: unwrapped from the signed container when it is one, and the
/// input untouched when it is already a bare executable.
fn to_elf(bytes: &[u8]) -> Vec<u8> {
    match selfish_container::Container::parse(bytes) {
        Ok(container) => container.to_elf().unwrap_or_else(|_| bytes.to_vec()),
        Err(_) => bytes.to_vec(),
    }
}

/// The `#` header, so a regenerated file states where its two halves came from without a
/// consumer having to be told out of band.
fn header(source: &str, total: usize, resolved: usize) -> String {
    format!(
        "# Export vaddrs, resolved to names.\n\
         #\n\
         # known_by = \"measured\"\n\
         # source = \"{source}\"\n\
         #\n\
         # Format: <name> <vaddr_hex>. Lines starting with '#' and blank lines are ignored.\n\
         #   vaddr  the export's offset within the module, measured from the file. A loader\n\
         #          reaches the function by adding it to the module's load base.\n\
         #   name   resolved from obscene's mined corpus (data/mined-names.txt) by the NID the\n\
         #          export table stores. {resolved} of {total} resolved; the rest keep their\n\
         #          encoded NID (hash#lib#mod), the honest statement that the name is not yet\n\
         #          known.\n\
         #\n\
         # Regenerate: obscene-tool vaddrs <module> --source \"{source}\"\n\
         #\n"
    )
}

/// Parse (`encoded_nid`, vaddr) pairs from an OBS report text stream.
pub fn parse_report_measures(text: &str) -> Vec<(String, u64)> {
    let mut pairs = Vec::new();
    for line in text.lines() {
        if line.starts_with("OBS|measure|139-exports/enumerate|") {
            let parts: Vec<&str> = line.split('|').collect();
            if let (Some(&encoded), Some(&vaddr_part)) = (parts.get(3), parts.get(5)) {
                let vaddr_str = vaddr_part.trim_start_matches("0x");
                if let Ok(vaddr) = u64::from_str_radix(vaddr_str, 16) {
                    pairs.push((encoded.to_owned(), vaddr));
                }
            }
        }
    }
    pairs
}

/// Emit the resolved export table to stdout, header first. Returns `(emitted, resolved)`.
///
/// # Errors
///
/// If the corpus or module cannot be read, the module is not an executable, or it carries no
/// vendor tables to read exports from.
pub fn run(
    module: &Path,
    corpus: &Path,
    source: &str,
) -> Result<(usize, usize), Box<dyn std::error::Error>> {
    let names = corpus_names(corpus)?;
    let raw_bytes = std::fs::read(module)?;

    let mut pairs: Vec<(String, u64)> = Vec::new();
    if let Ok(text) = std::str::from_utf8(&raw_bytes) {
        let from_report = parse_report_measures(text);
        if !from_report.is_empty() {
            pairs = from_report;
        }
    }

    if pairs.is_empty() {
        let inner = to_elf(&raw_bytes);
        let elf = selfish_elf::Elf::parse(&inner)?;
        let (blob, info) = elf
            .tables()?
            .ok_or("the module has no vendor tables to read exports from")?;
        let symbols = selfish_elf::dynamic::symbols(blob, &info)?;

        let at = usize::try_from(info.strtab).unwrap_or(0);
        let end = at
            .saturating_add(usize::try_from(info.strsz).unwrap_or(0))
            .min(blob.len());
        let strings = blob.get(at..end).unwrap_or(&[]);

        for symbol in symbols {
            // A defined export with an address: section nonzero, value nonzero.
            if symbol.section == 0 || symbol.value == 0 {
                continue;
            }
            let name_at = usize::try_from(symbol.name_offset).unwrap_or(0);
            let Some(rest) = strings.get(name_at..) else {
                continue;
            };
            let stop = rest.iter().position(|b| *b == 0).unwrap_or(rest.len());
            let encoded = String::from_utf8_lossy(rest.get(..stop).unwrap_or(rest)).into_owned();
            pairs.push((encoded, symbol.value));
        }
    }

    let mut lines: Vec<String> = Vec::new();
    let mut resolved: usize = 0;
    for (encoded, vaddr) in pairs {
        let head = encoded.split('#').next().unwrap_or(&encoded);
        let label = selfish_nid::Nid::decode(head)
            .ok()
            .map(|nid| format!("{:#018x}", nid.value()))
            .and_then(|key| names.get(&key).cloned())
            .map_or_else(
                || encoded.clone(),
                |name| {
                    resolved = resolved.saturating_add(1);
                    name
                },
            );
        lines.push(format!("{label} 0x{vaddr:x}"));
    }

    let total = lines.len();
    print!("{}", header(source, total, resolved));
    for line in &lines {
        println!("{line}");
    }
    Ok((total, resolved))
}

#[cfg(test)]
mod tests {
    use super::{parse_corpus, parse_report_measures};

    #[test]
    fn indexes_names_by_nid_skipping_comments() {
        // The real column shape: name, libraries, identifiers, sources, kind.
        let text = "\
# a header line, ignored
getpid libKernel,libkernel 0x1e82d558d6a70417 ps4libdoc fn
write libScePosix 0x14de2068f9ae155f,0x171559a81000ee4b shadPS4 fn
";
        let names = parse_corpus(text);
        // Resolved by the raw NID the export table carries, in the corpus's own lower-case form.
        assert_eq!(
            names.get("0x1e82d558d6a70417").map(String::as_str),
            Some("getpid")
        );
        // A name with two NIDs is reachable by either.
        assert_eq!(
            names.get("0x14de2068f9ae155f").map(String::as_str),
            Some("write")
        );
        assert_eq!(
            names.get("0x171559a81000ee4b").map(String::as_str),
            Some("write")
        );
        assert_eq!(names.len(), 3);
    }

    #[test]
    fn first_name_wins_for_a_shared_nid() {
        // Two names claiming one NID: the first in file order is kept, so the result does not
        // depend on map iteration order.
        let text = "alpha lib 0xdead src fn\nbeta lib 0xdead src fn\n";
        assert_eq!(
            parse_corpus(text).get("0xdead").map(String::as_str),
            Some("alpha")
        );
    }

    #[test]
    fn parses_report_measures() {
        let report = "\
OBS|section|139-exports|Exports|Where libkernel is
OBS|try|139-exports/enumerate|libkernel|enumerate
OBS|measure|139-exports/enumerate|HoLVWNanBBc#I#A|vaddr|0x5b0|offset
OBS|measure|139-exports/enumerate|sXzD8jK2dGs#I#A|vaddr|0x16e00|offset
OBS|res|139-exports/enumerate|pass|0x74b||hardware
";
        let pairs = parse_report_measures(report);
        assert_eq!(pairs.len(), 2);
        assert_eq!(pairs[0], ("HoLVWNanBBc#I#A".to_owned(), 0x5b0));
        assert_eq!(pairs[1], ("sXzD8jK2dGs#I#A".to_owned(), 0x16e00));
    }
}
