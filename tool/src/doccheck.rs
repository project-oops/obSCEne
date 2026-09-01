//! Documentation that names something which does not exist.
//!
//! Prose rots in two ways. It quotes a number that changes - which `obscene-tool counts`
//! owns - or it names a file, a make rule or a subcommand that has since moved. This is the
//! second kind, and it has caught `make target` named in four files without ever having
//! been a rule.
//!
//! # Only inside code spans, for the rules and subcommands
//!
//! Matching bare prose turns "make the comment true" and "make it fail" into missing build
//! rules, and a checker that cries wolf gets switched off - which would leave the real
//! `make target` in exactly the state that let it survive in four files.
//!
//! # `DECISIONS.md` and `WORKLOG.md` are exempt, and that is not laziness
//!
//! They are dated records. A count inside them was true when written and correcting it
//! afterwards is falsifying a log. But **a decision number is not prose**: the log's whole
//! value is that `see D105` identifies one decision, so numbering is gated separately and
//! the exemption does not cover it.

use std::collections::{BTreeMap, BTreeSet};
use std::path::{Path, PathBuf};

/// Files whose contents are dated records rather than descriptions of the tree.
const EXEMPT: [&str; 2] = ["DECISIONS.md", "WORKLOG.md"];

/// Subcommands the backlog proposes and the tool does not have yet.
///
/// Listed rather than exempting the file, so a proposal that ships has to be removed from
/// here. The Python said that and did not enforce it - a name in both lists was silently
/// accepted forever. Here a `PROPOSED` name that now exists is itself reported, which is
/// what the comment always claimed.
const PROPOSED: [&str; 0] = [];

/// Case-insensitive extension test.
///
/// Clippy asks for this rather than `ends_with(".md")`, and it is right to: the tree is
/// developed on a case-insensitive filesystem, so a `README.MD` would be a real file the
/// checks silently skipped.
fn has_extension(name: &str, extension: &str) -> bool {
    Path::new(name)
        .extension()
        .is_some_and(|e| e.eq_ignore_ascii_case(extension))
}

/// Every markdown file that describes the tree.
fn documented_files(root: &Path) -> std::io::Result<Vec<PathBuf>> {
    let mut out = Vec::new();
    let mut stack = vec![root.to_path_buf()];
    while let Some(dir) = stack.pop() {
        for entry in std::fs::read_dir(&dir)?.filter_map(Result::ok) {
            let path = entry.path();
            let name = entry.file_name().to_string_lossy().into_owned();
            if path.is_dir() {
                if !matches!(
                    name.as_str(),
                    ".git" | "target" | "build" | "reports" | "__pycache__" | ".github"
                ) {
                    stack.push(path);
                }
            } else if has_extension(&name, "md") && !EXEMPT.contains(&name.as_str()) {
                out.push(path);
            }
        }
    }
    out.sort();
    Ok(out)
}

/// The text inside backticks and fenced blocks, joined by newlines.
///
/// Newlines rather than spaces: adjacent spans `make` and `make check` concatenated with a
/// space read as the command "make make", a false positive of exactly the kind that gets a
/// checker switched off.
fn code_spans(text: &str) -> String {
    let mut out = String::new();
    // Fenced blocks first, so their contents are not re-read as inline spans.
    let mut rest = text;
    while let Some((_, after)) = rest.split_once("```") {
        let Some((block, tail)) = after.split_once("```") else {
            break;
        };
        // Drop the language tag on the opening fence.
        let block = block.split_once('\n').map_or(block, |(_, body)| body);
        out.push_str(block);
        out.push('\n');
        rest = tail;
    }
    for (index, span) in text.split('`').enumerate() {
        if index % 2 == 1 && !span.contains('\n') {
            out.push_str(span);
            out.push('\n');
        }
    }
    out
}

/// Words following a marker inside code spans - `make x`, `obscene-tool y`.
///
/// # Only where the marker begins a command
///
/// It used to split on the marker anywhere it appeared, which reads the middle of a sentence
/// as an invocation. `apt-get install clang lld make binutils gcc` was reported as
/// `make binutils` - no such rule, correctly, because nobody ever claimed there was one. The
/// gate was right about the fact and wrong about there being a claim, which is the more
/// annoying half: a false positive here trains people to ignore the one gate whose whole job
/// is catching prose that has drifted from the code.
///
/// So the character before the marker has to be one that ends a command: a newline, a shell
/// separator, or the `--` that precedes a command passed to something else.
fn words_after(code: &str, marker: &str) -> BTreeSet<String> {
    let mut out = BTreeSet::new();
    for (at, _) in code.match_indices(marker) {
        let before = code.get(..at).unwrap_or("");
        let previous = before.trim_end_matches([' ', '\t']).chars().last();
        let begins_a_command = match previous {
            None => true,
            Some(c) => matches!(c, '\n' | ';' | '&' | '|' | '(' | '`' | '$' | '#' | '-'),
        };
        if !begins_a_command {
            continue;
        }
        let rest = code.get(at.saturating_add(marker.len())..).unwrap_or("");
        let word: String = rest
            .chars()
            .take_while(|c| c.is_alphanumeric() || *c == '-' || *c == '_')
            .collect();
        if !word.is_empty() && word.chars().next().is_some_and(char::is_lowercase) {
            out.insert(word);
        }
    }
    out
}

/// Paths of the form `prefix/name` mentioned anywhere in the text.
fn paths_named(text: &str, prefix: &str) -> BTreeSet<String> {
    let mut out = BTreeSet::new();
    for chunk in text.split(prefix).skip(1) {
        let name: String = chunk
            .chars()
            .take_while(|c| c.is_alphanumeric() || matches!(c, '.' | '-' | '_' | '/'))
            .collect();
        let name = name.trim_end_matches('.').to_owned();
        if !name.is_empty() {
            out.insert(name);
        }
    }
    out
}

/// Rule names the Makefile defines.
fn make_rules(makefile: &str) -> BTreeSet<String> {
    let mut out = BTreeSet::new();
    for line in makefile.lines() {
        let Some((target, _)) = line.split_once(':') else {
            continue;
        };
        if !target.is_empty()
            && target.chars().next().is_some_and(char::is_lowercase)
            && target
                .chars()
                .all(|c| c.is_alphanumeric() || c == '-' || c == '_')
        {
            out.insert(target.to_owned());
        }
    }
    out
}

/// Subcommands, read from the clap enum where they are actually defined.
fn tool_subcommands(main_rs: &str) -> BTreeSet<String> {
    let mut out = BTreeSet::new();
    let Some((_, after)) = main_rs.split_once("enum Command {") else {
        return out;
    };
    let block = after.split_once("\n}").map_or(after, |(b, _)| b);
    for line in block.lines() {
        let trimmed = line.trim_start();
        // Variants sit at one indent level; anything deeper is a field.
        let indent = line.len().saturating_sub(trimmed.len());
        if indent != 4 {
            continue;
        }
        let name: String = trimmed
            .chars()
            .take_while(|c| c.is_alphanumeric() || *c == '_')
            .collect();
        if name.is_empty() || !name.chars().next().is_some_and(char::is_uppercase) {
            continue;
        }
        let rest = trimmed.get(name.len()..).unwrap_or_default().trim_start();
        if rest.starts_with('{') || rest.starts_with('(') || rest.starts_with(',') {
            let mut lowered = name.clone();
            lowered.replace_range(..1, &name[..1].to_lowercase());
            out.insert(lowered);
        }
    }
    out
}

/// `docs/README.md` lists every document, and nothing it lists has been deleted.
///
/// The index had drifted to five entries out of seventeen. That is the failure worth gating
/// rather than the missing links themselves: an index reads as complete, so a reader who does
/// not find a subject in it concludes the project has nothing on the subject.
///
/// Both directions are checked. A file added without a pointer is the common case; a pointer
/// left behind by a rename is the one that produces a dead link in the first document
/// anybody opens.
fn docs_index(root: &Path) -> std::io::Result<Vec<String>> {
    let mut problems = Vec::new();
    let docs = root.join("docs");
    let index_path = docs.join("README.md");
    if !index_path.is_file() {
        return Ok(problems);
    }
    let index = std::fs::read_to_string(&index_path)?;

    let mut listed = BTreeSet::new();
    for chunk in index.split('(').skip(1) {
        let Some((inner, _)) = chunk.split_once(')') else {
            continue;
        };
        if has_extension(inner, "md") && !inner.contains('/') {
            listed.insert(inner.to_owned());
        }
    }

    let mut present = BTreeSet::new();
    for entry in std::fs::read_dir(&docs)?.filter_map(Result::ok) {
        let name = entry.file_name().to_string_lossy().into_owned();
        if has_extension(&name, "md") && name != "README.md" {
            present.insert(name);
        }
    }

    for name in present.difference(&listed) {
        problems.push(format!("docs/README.md: {name} exists and is not listed"));
    }
    for name in listed.difference(&present) {
        problems.push(format!(
            "docs/README.md: links {name}, which does not exist"
        ));
    }
    Ok(problems)
}

/// Every decision has its own number, and every `D<n>` cited resolves to one.
///
/// Two numbering streams once left D102 to D114 spread over two or three unrelated decisions
/// apiece - seventeen entries sharing thirteen numbers, and a hundred citations that could no
/// longer be resolved without reading every candidate.
///
/// Nothing failed loudly, which is the point. A duplicate number is not a broken link: every
/// citation still finds *an* entry, just not reliably the right one, and the log reads as
/// intact until somebody follows a reference and gets the wrong decision.
fn decision_numbers(root: &Path, files: &[PathBuf]) -> std::io::Result<Vec<String>> {
    let mut problems = Vec::new();
    let path = root.join("docs").join("DECISIONS.md");
    if !path.is_file() {
        return Ok(problems);
    }
    let text = std::fs::read_to_string(&path)?;

    // Two heading styles: the early entries carry a title after the number, the later ones
    // leave the heading bare. Anchoring on the number and stopping there accepts both.
    // Insisting on end-of-line looked correct and silently skipped the first twenty-six.
    let mut seen: BTreeMap<u32, usize> = BTreeMap::new();
    let mut spelling: BTreeMap<u32, String> = BTreeMap::new();
    for line in text.lines() {
        let Some(rest) = line.strip_prefix("## D") else {
            continue;
        };
        let digits: String = rest.chars().take_while(char::is_ascii_digit).collect();
        let Ok(number) = digits.parse::<u32>() else {
            continue;
        };
        let count = seen.entry(number).or_insert(0);
        *count = count.saturating_add(1);
        // Reported as written: saying "D42" of a heading that reads `## D042` costs the
        // reader a failed grep before they find what is being complained about.
        spelling.entry(number).or_insert(digits);
    }
    for (number, count) in &seen {
        if *count > 1 {
            let written = spelling.get(number).cloned().unwrap_or_default();
            problems.push(format!(
                "docs/DECISIONS.md: D{written} heads {count} different decisions - \
                 a citation cannot resolve"
            ));
        }
    }

    // Citations, from everywhere. Zero-padding is spelling, not identity: `D008` and `D8`
    // are the same decision and both are in the tree.
    for path in files {
        let rel = path
            .strip_prefix(root)
            .unwrap_or(path)
            .to_string_lossy()
            .replace('\\', "/");
        let text = std::fs::read_to_string(path)?;
        let mut cited = BTreeSet::new();
        for chunk in text.split('D').skip(1) {
            let digits: String = chunk.chars().take_while(char::is_ascii_digit).collect();
            if digits.is_empty() || digits.len() > 3 {
                continue;
            }
            let next = chunk.chars().nth(digits.len());
            if next.is_some_and(|c| c.is_alphanumeric() || c == '_') {
                continue;
            }
            if let Ok(number) = digits.parse::<u32>() {
                cited.insert((number, digits));
            }
        }
        for (number, written) in cited {
            if !seen.contains_key(&number) {
                problems.push(format!("{rel}: D{written} - no such decision"));
            }
        }
    }
    Ok(problems)
}

/// Run every documentation check. Returns the problems found, empty when all is well.
pub fn run(root: &Path) -> std::io::Result<Vec<String>> {
    let mut problems = Vec::new();
    let files = documented_files(root)?;

    let rules = make_rules(&std::fs::read_to_string(root.join("Makefile"))?);
    let subcommands = tool_subcommands(&std::fs::read_to_string(
        root.join("tool").join("src").join("main.rs"),
    )?);

    for name in PROPOSED {
        if subcommands.contains(name) {
            problems.push(format!(
                "scripts: `{name}` is listed as proposed but the tool now has it - \
                 remove it from PROPOSED"
            ));
        }
    }

    for path in &files {
        let rel = path
            .strip_prefix(root)
            .unwrap_or(path)
            .to_string_lossy()
            .replace('\\', "/");
        let text = std::fs::read_to_string(path)?;
        let code = code_spans(&text);

        for rule in words_after(&code, "make ") {
            if rule.contains('=') || rules.contains(&rule) {
                continue;
            }
            problems.push(format!("{rel}: `make {rule}` - no such rule"));
        }

        for script in paths_named(&text, "scripts/") {
            if !root.join("scripts").join(&script).exists() {
                problems.push(format!("{rel}: scripts/{script} - no such file"));
            }
        }

        for sub in words_after(&code, "obscene-tool ") {
            if subcommands.contains(&sub) || PROPOSED.contains(&sub.as_str()) {
                continue;
            }
            problems.push(format!("{rel}: `obscene-tool {sub}` - no such subcommand"));
        }

        // A doc pointing at one of our source files that has moved is the same fault. Only
        // inside code spans: an emulator's `src/loader/elf.h` quoted in ACKNOWLEDGEMENTS is
        // somebody else's file and not ours to resolve.
        for src in paths_named(&code, "src/") {
            if !(has_extension(&src, "c") || has_extension(&src, "h")) {
                continue;
            }
            if !root.join("src").join(&src).exists() {
                problems.push(format!("{rel}: src/{src} - no such file"));
            }
        }
    }

    problems.extend(decision_numbers(root, &files)?);
    problems.extend(docs_index(root)?);
    problems.sort();
    problems.dedup();
    Ok(problems)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn only_code_spans_are_read_for_rules() {
        let code = code_spans("Run `make host` to build. But make the comment true first.");
        let rules = words_after(&code, "make ");
        assert!(rules.contains("host"));
        assert!(
            !rules.contains("the"),
            "prose must not be read as a build rule"
        );
    }

    /// Adjacent spans joined by a space read as `make make`, which is the false positive
    /// that gets a checker switched off.
    #[test]
    fn adjacent_spans_do_not_merge() {
        let code = code_spans("`make` and `make check`");
        let rules = words_after(&code, "make ");
        assert!(rules.contains("check"));
        assert_eq!(rules.len(), 1, "got {rules:?}");
    }

    #[test]
    fn make_rules_come_from_targets_not_variables() {
        let rules = make_rules("host: deps\n\tcc\nCORPUS ?= 1\n.PHONY: all\nall: host\n");
        assert!(rules.contains("host"));
        assert!(rules.contains("all"));
        assert!(!rules.contains("CORPUS"));
    }

    #[test]
    fn subcommands_come_from_the_clap_enum() {
        let text = "enum Command {\n    /// doc\n    Nid {\n        file: PathBuf,\n    },\n    Selftest,\n}\n";
        let subs = tool_subcommands(text);
        assert!(subs.contains("nid"));
        assert!(subs.contains("selftest"));
        assert!(!subs.contains("file"), "a field is not a subcommand");
    }

    #[test]
    fn a_duplicate_decision_number_is_reported() {
        let dir = std::env::temp_dir().join("obscene-doccheck-dup");
        let docs = dir.join("docs");
        std::fs::create_dir_all(&docs).expect("temp dir");
        std::fs::write(
            docs.join("DECISIONS.md"),
            "## D042\n\nfirst\n\n## D042\n\nsecond\n",
        )
        .expect("write");
        let problems = decision_numbers(&dir, &[]).expect("scan");
        assert_eq!(problems.len(), 1, "got {problems:?}");
        assert!(problems.iter().any(|p| p.contains("D042")));
        std::fs::remove_dir_all(&dir).ok();
    }

    /// Zero-padding is spelling, not identity, and both forms are in the tree.
    #[test]
    fn padded_and_bare_citations_are_the_same_decision() {
        let dir = std::env::temp_dir().join("obscene-doccheck-pad");
        let docs = dir.join("docs");
        std::fs::create_dir_all(&docs).expect("temp dir");
        std::fs::write(docs.join("DECISIONS.md"), "## D008\n\nonly\n").expect("write");
        let citing = dir.join("CITE.md");
        std::fs::write(&citing, "see D8 and D008 and D009\n").expect("write");
        let problems = decision_numbers(&dir, &[citing]).expect("scan");
        assert_eq!(problems.len(), 1, "got {problems:?}");
        assert!(problems.iter().any(|p| p.contains("D009")));
        std::fs::remove_dir_all(&dir).ok();
    }
}

#[cfg(test)]
mod words_after_tests {
    use super::words_after;

    #[test]
    fn a_marker_mid_sentence_is_not_an_invocation() {
        // The false positive this was changed for: a package list that happens to contain
        // `make` as one of its packages.
        let code = "sudo apt-get install clang lld make binutils gcc libc6-dev";
        assert!(
            words_after(code, "make ").is_empty(),
            "a package named make in an install list is not a make invocation"
        );
    }

    #[test]
    fn a_real_invocation_is_still_caught() {
        // The half that matters. Loosening a gate is only safe if the thing it was built for
        // still trips it, so this is the negative test for the change above.
        assert!(words_after("make check BUILD=/tmp/obs", "make ").contains("check"));
        assert!(words_after("cd tool && make host", "make ").contains("host"));
        assert!(words_after("wsl -- make module", "make ").contains("module"));
        assert!(words_after("x\nmake payload", "make ").contains("payload"));
    }

    #[test]
    fn a_pipeline_or_subshell_still_counts_as_a_command() {
        assert!(words_after("foo | make diff", "make ").contains("diff"));
        assert!(words_after("$(make pretty)", "make ").contains("pretty"));
        assert!(words_after("a; make clean", "make ").contains("clean"));
    }
}
