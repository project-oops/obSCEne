//! The GPU ISA surface census: the hardware's math operations, and obSCEne's reach into each.
//!
//! This is the GPU analogue of the CPU surface census. "Probe every GPU op" needs a statement
//! of what every GPU op *is*, and this is that statement - the list the hardware day diffs
//! against.
//!
//! # The table is data, not code
//!
//! The classification lives in `data/gpu-surface.tsv` rather than in this file. It is a
//! judgement about which hardware instruction a GLSL call lowers to, sixty rows of it with
//! prose attached, and transcribing that into a new language is how a transcription error
//! gets into a census. It was relocated rather than retyped when the generator moved from
//! Python, which cost nothing and risked nothing.
//!
//! # Two authorities, and only one is required
//!
//! The names are LLVM's, from the open-source `IntrinsicsAMDGPU.td`. When that file is
//! installed the census is cross-checked against it - every classified name must still exist,
//! and every scalar-math intrinsic must be classified - but generation stands without it. The
//! coverage claims are checked against `src/shaders/*.comp` unconditionally, because a
//! `covered:` claim naming a kernel that does not exist is a lie the document can tell on any
//! machine.
//!
//! **Regenerate where the `.td` is.** Without it the document gains a line saying names were
//! not cross-checked this run - which is honest, and different from the committed version,
//! so the gate then fails on a machine that *does* have it. The build VM has it; a Windows
//! host does not. Inherited from the Python and worth stating rather than rediscovering.

use std::collections::{BTreeMap, BTreeSet};
use std::path::Path;

/// One row of the classification table.
pub struct Row {
    /// Which table it belongs to: `census`, `base` or `int`.
    pub table: String,
    /// The operation's name.
    pub name: String,
    /// Functional unit, for the intrinsic census only.
    pub unit: String,
    /// Whether the RDNA2 target has it: `yes`, `no` or `?`.
    pub on_rdna: String,
    /// How obSCEne reaches it: `covered:<kernel>`, `glsl`, `sequence`, `isa`, `na-cdna`.
    pub reach: String,
    /// What the operation is, and why it is classified this way.
    pub note: String,
}

/// Read `data/gpu-surface.tsv`.
pub fn table(root: &Path) -> std::io::Result<Vec<Row>> {
    let text = std::fs::read_to_string(root.join("data").join("gpu-surface.tsv"))?;
    let mut out = Vec::new();
    for line in text.lines() {
        if line.starts_with('#') || line.trim().is_empty() {
            continue;
        }
        let f: Vec<&str> = line.split('\t').collect();
        let (Some(table), Some(name), Some(unit), Some(approx), Some(reach), Some(note)) =
            (f.first(), f.get(1), f.get(2), f.get(3), f.get(4), f.get(5))
        else {
            continue;
        };
        out.push(Row {
            table: (*table).to_owned(),
            name: (*name).to_owned(),
            unit: (*unit).to_owned(),
            // Tri-state, taken verbatim: yes, no, or `?` for an operation nobody has
            // established the target has. Collapsing the third into either of the others
            // is how the document came to claim six instructions it could not vouch for.
            on_rdna: (*approx).to_owned(),
            reach: (*reach).to_owned(),
            note: (*note).to_owned(),
        });
    }
    Ok(out)
}

/// The kernel names obSCEne actually has, so a `covered:` claim is checked against reality
/// rather than asserted.
fn kernels(root: &Path) -> BTreeSet<String> {
    let dir = root.join("src").join("shaders");
    std::fs::read_dir(dir)
        .into_iter()
        .flatten()
        .filter_map(Result::ok)
        .filter(|e| {
            e.path()
                .extension()
                .is_some_and(|x| x.eq_ignore_ascii_case("comp"))
        })
        .filter_map(|e| {
            e.path()
                .file_stem()
                .and_then(|s| s.to_str())
                .map(str::to_owned)
        })
        .collect()
}

/// Intrinsic names the toolchain models, when `IntrinsicsAMDGPU.td` is installed.
fn td_names(td: &Path) -> Option<BTreeSet<String>> {
    let text = std::fs::read_to_string(td).ok()?;
    let mut out = BTreeSet::new();
    for line in text.lines() {
        let Some(rest) = line.strip_prefix("def ") else {
            continue;
        };
        let name: String = rest
            .chars()
            .take_while(|c| c.is_ascii_lowercase() || c.is_ascii_digit() || *c == '_')
            .collect();
        if name.starts_with("int_amdgcn_") {
            out.insert(name);
        }
    }
    Some(out)
}

/// Scalar-math intrinsic families this census is responsible for classifying.
///
/// **Matched exactly, never by prefix.** `int_amdgcn_rcp` is this census's responsibility and
/// `int_amdgcn_rcp_f16` is not; a prefix test claims both and reports a false gap every time
/// the toolchain adds a half-precision variant. The list lives in the data file with the
/// classification it belongs to - it was an anchored regex alternation in the Python, and
/// re-typing forty names into a new language is the transcription error this avoids.
fn families(rows: &[Row]) -> BTreeSet<String> {
    rows.iter()
        .filter(|r| r.table == "family")
        .map(|r| format!("int_amdgcn_{}", r.name))
        .collect()
}

/// Stale classifications, unclaimed new operations, and coverage claims with nothing behind
/// them.
pub fn validate(
    rows: &[Row],
    have: &BTreeSet<String>,
    present: Option<&BTreeSet<String>>,
) -> Vec<String> {
    let mut problems = Vec::new();

    // Every `covered:<kernel>` must name a kernel that exists.
    for row in rows {
        let Some(kernel) = row.reach.strip_prefix("covered:") else {
            continue;
        };
        if !have.contains(kernel) {
            problems.push(format!(
                "{} claims covered:{kernel}, but src/shaders/{kernel}.comp does not exist",
                row.name
            ));
        }
    }

    let Some(present) = present else {
        // No `.td` installed: skip the cross-check. Generation still stands.
        return problems;
    };

    let classified: BTreeSet<String> = rows
        .iter()
        .filter(|r| r.table == "census")
        .map(|r| format!("int_amdgcn_{}", r.name))
        .collect();

    for name in &classified {
        if !present.contains(name) {
            problems.push(format!(
                "{name} is in the census but not in IntrinsicsAMDGPU.td - the classification \
                 is stale"
            ));
        }
    }
    // Any scalar-math intrinsic the toolchain has and this census does not is a new operation
    // to place - the drift this check exists to catch.
    let ours = families(rows);
    for name in present {
        if ours.contains(name) && !classified.contains(name) {
            problems.push(format!(
                "{name} is a scalar-math intrinsic in IntrinsicsAMDGPU.td with no census \
                 entry - classify it"
            ));
        }
    }
    problems
}

/// Where an operation sits in the document's ordering.
fn unit_order(unit: &str) -> usize {
    match unit {
        "sfu" => 0,
        "division" => 1,
        "decompose" => 2,
        "fused" => 3,
        "median" => 4,
        "convert" => 5,
        "cubemap" => 6,
        _ => 9,
    }
}

/// The prose preamble and the coverage summary.
///
/// Split from [`render`] because together they tripped the function-length lint, and the
/// two are read separately anyway: this is the part a person reads, the tables below are
/// the part a diff reads.
fn preamble(census: &[&Row], cross_checked: bool) -> Vec<String> {
    let mut reached: BTreeMap<&str, usize> = BTreeMap::new();
    for row in census {
        let kind = row.reach.split(':').next().unwrap_or_default();
        let count = reached.entry(kind).or_insert(0);
        *count = count.saturating_add(1);
    }
    let total = census.len();
    let probeable = census.iter().filter(|r| r.reach != "na-cdna").count();
    let count = |kind: &str| reached.get(kind).copied().unwrap_or(0);

    let mut out = vec![
        "# The GPU ISA surface, and obSCEne's coverage of it".to_owned(),
        String::new(),
        "<!-- Generated by obscene-tool gpusurface. Do not edit by hand; run".to_owned(),
        "     `obscene-tool gpusurface --write`. The source of truth is".to_owned(),
        "     data/gpu-surface.tsv, cross-checked against LLVM's".to_owned(),
        "     IntrinsicsAMDGPU.td when it is installed. -->".to_owned(),
        String::new(),
        "This is the GPU analogue of the CPU surface census (`surface.h`, `corpus.h`): the AMD \
         hardware's special math operations, and how obSCEne reaches each one. It is the list \
         the hardware day diffs against - \"probe every GPU op\" needs a statement of what \
         every GPU op *is*."
            .to_owned(),
        String::new(),
        "The authority for the names is LLVM's open-source `IntrinsicsAMDGPU.td`. The standard \
         IEEE operations (`floor`, `add`, `mul`, `fma`, `min`, `max`, the roundings) are not \
         AMDGPU intrinsics and are listed separately under [base ISA](#base-isa-not-intrinsics); \
         the transcendentals, the division sequence, decomposition and the packing conversions \
         are the intrinsic surface below."
            .to_owned(),
        String::new(),
        "## How obSCEne reaches an op".to_owned(),
        String::new(),
        "- **covered** - an existing kernel's GLSL lowers to this op on RDNA.".to_owned(),
        "- **glsl** - reachable through a GLSL builtin obSCEne does not yet call: a kernel to \
         add."
            .to_owned(),
        "- **sequence** - not isolable on its own; reached only as the driver's decomposition \
         of a full-precision operation (the division primitives inside divf/rcp), whose \
         end-to-end result the reference already checks."
            .to_owned(),
        "- **isa** - not reachable from GLSL or as a sequence; would need hand-written SPIR-V \
         or the intrinsic directly."
            .to_owned(),
        "- **na-cdna** - CDNA/MI parts only, not the RDNA2 (gfx1033) in the Steam Deck; nothing \
         to probe on the target."
            .to_owned(),
        String::new(),
        "## Coverage".to_owned(),
        String::new(),
        format!(
            "- **{total}** intrinsic operations enumerated, of which **{probeable}** exist on \
             the RDNA2 target."
        ),
        format!("- **{}** covered by an existing kernel.", count("covered")),
        format!(
            "- **{}** reachable from GLSL but not yet probed - the next kernels to add.",
            count("glsl")
        ),
        format!(
            "- **{}** reached only as the decomposition of full-precision division, not \
             isolable.",
            count("sequence")
        ),
        format!(
            "- **{}** reachable only through hand-written SPIR-V.",
            count("isa")
        ),
        format!("- **{}** absent on the RDNA2 target.", count("na-cdna")),
    ];

    if !cross_checked {
        out.push(String::new());
        out.push(
            "> Generated without `IntrinsicsAMDGPU.td` installed, so names were not \
             cross-checked against the toolchain this run."
                .to_owned(),
        );
    }
    out
}

/// The census document.
#[must_use]
pub fn render(rows: &[Row], cross_checked: bool) -> String {
    let census: Vec<&Row> = rows.iter().filter(|r| r.table == "census").collect();
    let mut out = preamble(&census, cross_checked);
    out.push(String::new());
    out.push("## The intrinsic surface".to_owned());
    out.push(String::new());
    out.push("| operation | category | on RDNA2 | obSCEne reaches it | note |".to_owned());
    out.push("|---|---|---|---|---|".to_owned());
    let mut sorted = census.clone();
    sorted.sort_by(|a, b| {
        unit_order(&a.unit)
            .cmp(&unit_order(&b.unit))
            .then_with(|| a.name.cmp(&b.name))
    });
    for row in sorted {
        out.push(format!(
            "| `{}` | {} | {} | {} | {} |",
            row.name, row.unit, row.on_rdna, row.reach, row.note
        ));
    }

    for (table, heading, blurb) in [
        (
            "base",
            "## Base ISA (not intrinsics)",
            "These obSCEne covers, but LLVM models them as generic float operations rather than \
             AMDGPU intrinsics, so they carry no `int_amdgcn_` name and are not part of the \
             intrinsic surface above.",
        ),
        (
            "int",
            "## Integer / bit base ISA",
            "Integer and bit operations, likewise generic LLVM operations rather than AMDGPU \
             intrinsics, but each lowering to a dedicated RDNA integer instruction. All exact - \
             the device must match bit for bit. This surface is the integer/bit breadth beyond \
             the float intrinsic census, and grows as kernels are added.",
        ),
    ] {
        out.push(String::new());
        out.push(heading.to_owned());
        out.push(String::new());
        out.push(blurb.to_owned());
        out.push(String::new());
        out.push("| operation | obSCEne reaches it | note |".to_owned());
        out.push("|---|---|---|".to_owned());
        for row in rows.iter().filter(|r| r.table == table) {
            out.push(format!("| `{}` | {} | {} |", row.name, row.reach, row.note));
        }
    }
    out.push(String::new());
    out.join("\n")
}

/// Validate, then write or check the document. Returns true when all is well.
pub fn run(root: &Path, td: &Path, write: bool, check: bool) -> std::io::Result<bool> {
    let rows = table(root)?;
    let have = kernels(root);
    let present = td_names(td);
    let problems = validate(&rows, &have, present.as_ref());
    if !problems.is_empty() {
        println!("the GPU surface census disagrees with the tree:");
        for line in &problems {
            println!("  {line}");
        }
        return Ok(false);
    }

    let fresh = render(&rows, present.is_some()) + "\n";
    let path = root.join("docs").join("GPU_SURFACE.md");
    if check {
        let current = std::fs::read_to_string(&path).unwrap_or_default();
        if current == fresh {
            println!("GPU surface census current ({} operations)", rows.len());
            return Ok(true);
        }
        println!("GPU_SURFACE.md is out of date - regenerate it");
        return Ok(false);
    }
    if write {
        std::fs::write(&path, fresh)?;
        println!("written: {}", path.display());
    } else {
        print!("{fresh}");
    }
    Ok(true)
}

#[cfg(test)]
mod tests {
    use super::*;

    fn row(table: &str, name: &str, reach: &str) -> Row {
        Row {
            table: table.to_owned(),
            name: name.to_owned(),
            unit: "sfu".to_owned(),
            on_rdna: "yes".to_owned(),
            reach: reach.to_owned(),
            note: "a note".to_owned(),
        }
    }

    /// A `covered:` claim naming a kernel that does not exist is a lie the document can tell
    /// on any machine, with or without the toolchain installed.
    #[test]
    fn a_coverage_claim_needs_a_kernel_behind_it() {
        let have: BTreeSet<String> = ["sqrt".to_owned()].into_iter().collect();
        let rows = vec![
            row("census", "sqrt", "covered:sqrt"),
            row("census", "rcp", "covered:rcp"),
        ];
        let problems = validate(&rows, &have, None);
        assert_eq!(problems.len(), 1, "got {problems:?}");
        assert!(problems.iter().any(|p| p.contains("covered:rcp")));
    }

    #[test]
    fn unreachable_classifications_need_no_kernel() {
        let have = BTreeSet::new();
        let rows = vec![
            row("census", "cubeid", "isa"),
            row("census", "mfma", "na-cdna"),
            row("census", "ldexp", "glsl"),
        ];
        assert!(validate(&rows, &have, None).is_empty());
    }

    /// The drift this exists to catch: the toolchain gains a scalar-math intrinsic and the
    /// census does not.
    #[test]
    fn an_unclassified_scalar_math_intrinsic_is_reported() {
        let have: BTreeSet<String> = ["sqrt".to_owned()].into_iter().collect();
        let mut rows = vec![row("census", "sqrt", "covered:sqrt")];
        rows.push(row("family", "sqrt", ""));
        rows.push(row("family", "rcp", ""));
        let present: BTreeSet<String> = [
            "int_amdgcn_sqrt".to_owned(),
            "int_amdgcn_rcp".to_owned(),
            // Not a family: a prefix test would wrongly claim this one.
            "int_amdgcn_rcp_f16".to_owned(),
        ]
        .into_iter()
        .collect();
        let problems = validate(&rows, &have, Some(&present));
        assert!(
            problems.iter().any(|p| p.contains("int_amdgcn_rcp is")),
            "got {problems:?}"
        );
        assert!(
            !problems.iter().any(|p| p.contains("rcp_f16")),
            "a half-precision variant is not this census's responsibility"
        );
    }

    #[test]
    fn a_classification_the_toolchain_dropped_is_reported() {
        let have: BTreeSet<String> = ["sqrt".to_owned()].into_iter().collect();
        let rows = vec![row("census", "sqrt", "covered:sqrt")];
        let present: BTreeSet<String> = BTreeSet::new();
        let problems = validate(&rows, &have, Some(&present));
        assert!(problems.iter().any(|p| p.contains("is stale")));
    }
}
