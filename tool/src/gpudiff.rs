//! Comparing two GPU observation corpora, lane by lane.
//!
//! # Why this is the point of the whole GPU effort
//!
//! obSCEne records what a device computed - `gpu` and `gpuop` records, bits in and bits out.
//! On its own that is inert. The value is in the *diff*: run the same kernels on real RDNA2
//! and on an emulator (or on an exact reference), and the lanes where the outputs disagree
//! are the shader gaps, named precisely. Without this, a hardware run is a pile of records
//! nobody can act on; with it, the hardware day ends in a list of exactly where the emulator
//! is wrong.
//!
//! # It compares by input, not by lane
//!
//! A lane number is just a position; the operands are the identity. Keying on `(kernel,
//! inputs)` means two corpora line up even if the lane order differs, and a divergence names
//! the operands that produced it - which is what a person fixing the emulator needs, not
//! "lane 7".
//!
//! # It prints both provenances, always
//!
//! Diffing an `llvmpipe` corpus against a `gfx1033` one is the intended use, and the tool
//! says which is which up front. A diff whose two sides are the same device is measuring
//! nondeterminism; a diff across devices is measuring the gap. The reader must be able to
//! tell them apart, so the `gpudev` line of each side is reported before any divergence.

use std::collections::BTreeMap;

/// One corpus, reduced to what a diff needs.
#[derive(Default)]
pub struct GpuCorpus {
    /// backend/device/type, verbatim from the `gpudev` record - the provenance of every
    /// result below. Empty if the corpus carried none.
    pub device: String,
    /// `(kernel, inputs)` -> output bits. Inputs are the hex fields exactly as recorded, so
    /// the key is a device-independent identity for a lane.
    pub lanes: BTreeMap<(String, Vec<String>), String>,
}

/// Canonicalises a hex field to a fixed `0x%08x` form.
///
/// Two corpora can print the same 32-bit value with different widths - obSCEne's report emits
/// minimal digits (`0x0`), the reference oracle emits `0x00000000` - and a string compare then
/// reads identical bits as a divergence. Canonicalising both to eight digits fixes that without
/// touching a real bit difference: `0xffc00000` and `0x7fc00000` are two NaNs with different
/// sign bits, and they stay two distinct values because they *are* two distinct values.
///
/// A field that is not a 32-bit hex value - a log fragment that landed in a value slot, which
/// the emulator runners warn does happen - is kept verbatim, so it still surfaces as a
/// divergence rather than being normalised into a false match.
fn canon(field: &str) -> String {
    let digits = field
        .strip_prefix("0x")
        .or_else(|| field.strip_prefix("0X"));
    match digits.and_then(|d| u32::from_str_radix(d, 16).ok()) {
        Some(v) => format!("0x{v:08x}"),
        None => field.to_string(),
    }
}

impl GpuCorpus {
    /// Reads the `gpu`, `gpuop` and `gpudev` records out of a corpus, ignoring everything
    /// else - so it reads a full report, a driver transcript, or a bare capture alike.
    pub fn parse(text: &str) -> GpuCorpus {
        let mut corpus = GpuCorpus::default();
        for line in text.lines() {
            let f: Vec<&str> = line.trim_end_matches(['\r', '\n']).split('|').collect();
            if f.first() != Some(&"OBS") {
                continue;
            }
            // Every field through `.get()` rather than by index: the record kinds vary in
            // length, so an index safe for one panics on another. The `>= N` guards keep the
            // logic honest; the getters keep a short or malformed line from taking the parse
            // down. (Production code - the tests opt into indexing separately.)
            let field = |n: usize| f.get(n).copied().unwrap_or("");
            match field(1) {
                // OBS|gpudev|backend|device|type
                "gpudev" if f.len() >= 4 => {
                    corpus.device = f.get(2..).unwrap_or(&[]).join("|");
                }
                // OBS|gpu|kernel|lane|input|output. Inputs and output canonicalised so a lane
                // keys and compares on its bits, not on how a producer chose to print them.
                "gpu" if f.len() >= 6 => {
                    let key = (field(2).to_string(), vec![canon(field(4))]);
                    corpus.lanes.insert(key, canon(field(5)));
                }
                // OBS|gpuop|kernel|lane|output|in0|in1...
                "gpuop" if f.len() >= 5 => {
                    let inputs: Vec<String> =
                        f.get(5..).unwrap_or(&[]).iter().map(|s| canon(s)).collect();
                    let key = (field(2).to_string(), inputs);
                    corpus.lanes.insert(key, canon(field(4)));
                }
                _ => {}
            }
        }
        corpus
    }
}

/// A single lane where two corpora disagree, or where one has a lane the other lacks.
#[derive(Debug, PartialEq, Eq)]
pub struct Divergence {
    pub kernel: String,
    pub inputs: Vec<String>,
    /// The output on the golden side, or `None` if this lane is absent there.
    pub golden: Option<String>,
    /// The output on the fresh side, or `None` if this lane is absent there.
    pub fresh: Option<String>,
}

/// Everything a comparison found.
pub struct GpuComparison {
    pub golden_device: String,
    pub fresh_device: String,
    /// Lanes present in both and equal - the agreement, counted not listed.
    pub agreed: usize,
    /// Lanes that differ, or that only one side has. Sorted, so output is deterministic.
    pub divergences: Vec<Divergence>,
}

/// Compares two corpora lane by lane.
///
/// A lane in both with equal output is agreement. A lane in both with different output is a
/// divergence - the interesting case, a device computing something the other did not. A lane
/// in only one side is also reported: it means the two runs did not cover the same ground
/// (a different kernel set or input vector), which is a real difference worth seeing rather
/// than silently dropping.
pub fn compare(golden: &GpuCorpus, fresh: &GpuCorpus) -> GpuComparison {
    let mut agreed: usize = 0;
    let mut divergences = Vec::new();

    for (key, g_out) in &golden.lanes {
        match fresh.lanes.get(key) {
            Some(f_out) if f_out == g_out => agreed = agreed.saturating_add(1),
            Some(f_out) => divergences.push(Divergence {
                kernel: key.0.clone(),
                inputs: key.1.clone(),
                golden: Some(g_out.clone()),
                fresh: Some(f_out.clone()),
            }),
            None => divergences.push(Divergence {
                kernel: key.0.clone(),
                inputs: key.1.clone(),
                golden: Some(g_out.clone()),
                fresh: None,
            }),
        }
    }
    // Lanes only the fresh side has.
    for (key, f_out) in &fresh.lanes {
        if !golden.lanes.contains_key(key) {
            divergences.push(Divergence {
                kernel: key.0.clone(),
                inputs: key.1.clone(),
                golden: None,
                fresh: Some(f_out.clone()),
            });
        }
    }
    divergences.sort_by(|a, b| (&a.kernel, &a.inputs).cmp(&(&b.kernel, &b.inputs)));

    GpuComparison {
        golden_device: golden.device.clone(),
        fresh_device: fresh.device.clone(),
        agreed,
        divergences,
    }
}

/// Renders a comparison. Provenance first, then a divergence per line.
pub fn render(cmp: &GpuComparison) -> String {
    use std::fmt::Write as _;
    let mut out = String::new();
    let _ = writeln!(
        out,
        "golden device: {}",
        if cmp.golden_device.is_empty() {
            "(none stated)"
        } else {
            &cmp.golden_device
        }
    );
    let _ = writeln!(
        out,
        "fresh device:  {}",
        if cmp.fresh_device.is_empty() {
            "(none stated)"
        } else {
            &cmp.fresh_device
        }
    );
    let _ = writeln!(
        out,
        "{} lanes agreed, {} diverged",
        cmp.agreed,
        cmp.divergences.len()
    );
    if cmp.golden_device == cmp.fresh_device && !cmp.golden_device.is_empty() {
        let _ = writeln!(
            out,
            "note: both sides are the same device, so a divergence is nondeterminism, not a gap"
        );
    }
    for d in &cmp.divergences {
        let g = d.golden.as_deref().unwrap_or("(absent)");
        let f = d.fresh.as_deref().unwrap_or("(absent)");
        let _ = writeln!(
            out,
            "  {}({}) golden={} fresh={}",
            d.kernel,
            d.inputs.join(","),
            g,
            f
        );
    }
    out
}

#[cfg(test)]
#[allow(
    clippy::indexing_slicing,
    reason = "test fixtures build known-size inputs; a panic here is the failure signal"
)]
mod tests {
    use super::*;

    #[test]
    fn identical_corpora_have_no_divergence() {
        let text = "OBS|gpudev|vulkan|llvmpipe|cpu\n\
                    OBS|gpu|rcp|0|0x40400000|0x3eaaaaab\n\
                    OBS|gpuop|divf|0|0x7f800000|0x3f800000|0x0\n";
        let a = GpuCorpus::parse(text);
        let b = GpuCorpus::parse(text);
        let cmp = compare(&a, &b);
        assert_eq!(cmp.agreed, 2);
        assert!(cmp.divergences.is_empty());
    }

    /// The case the tool exists for: same input, different output bits.
    #[test]
    fn a_different_output_bit_is_a_divergence() {
        let golden = GpuCorpus::parse("OBS|gpu|rcp|0|0x40400000|0x3eaaaaab\n");
        // The RDNA2 reciprocal approximation lands one ULP off the correctly-rounded value.
        let fresh = GpuCorpus::parse("OBS|gpu|rcp|0|0x40400000|0x3eaaaaaa\n");
        let cmp = compare(&golden, &fresh);
        assert_eq!(cmp.agreed, 0);
        assert_eq!(cmp.divergences.len(), 1);
        assert_eq!(cmp.divergences[0].kernel, "rcp");
        assert_eq!(cmp.divergences[0].golden.as_deref(), Some("0x3eaaaaab"));
        assert_eq!(cmp.divergences[0].fresh.as_deref(), Some("0x3eaaaaaa"));
    }

    /// Multi-operand lanes key on the whole input tuple, output-before-inputs layout.
    #[test]
    fn multi_operand_lanes_key_on_all_inputs() {
        let golden =
            GpuCorpus::parse("OBS|gpuop|fmaf|0|0x40400000|0x3f800000|0x40000000|0x3f800000\n");
        let fresh =
            GpuCorpus::parse("OBS|gpuop|fmaf|0|0x40400001|0x3f800000|0x40000000|0x3f800000\n");
        let cmp = compare(&golden, &fresh);
        assert_eq!(cmp.divergences.len(), 1);
        assert_eq!(
            cmp.divergences[0].inputs,
            vec!["0x3f800000", "0x40000000", "0x3f800000"]
        );
    }

    /// A lane only one side has is a divergence, not a silent drop.
    #[test]
    fn a_lane_only_one_side_has_is_reported() {
        let golden = GpuCorpus::parse("OBS|gpu|sin|0|0x40000000|0x3f68c7b7\n");
        let fresh = GpuCorpus::parse("");
        let cmp = compare(&golden, &fresh);
        assert_eq!(cmp.divergences.len(), 1);
        assert_eq!(cmp.divergences[0].fresh, None);
    }

    /// The bug the reference oracle exposed: obSCEne prints `0x0`, the oracle prints
    /// `0x00000000`, and a naive string compare called identical bits a divergence. They must
    /// agree - but a genuine bit difference in the same slot (two NaNs, different sign) must
    /// still diverge, which is what keeps canonicalisation from hiding a real finding.
    #[test]
    fn hex_width_is_normalised_but_real_bit_differences_survive() {
        let golden = GpuCorpus::parse(
            "OBS|gpu|absf|0|0x0|0x0\n\
             OBS|gpu|acos|1|0x40000000|0xffc00000\n",
        );
        let fresh = GpuCorpus::parse(
            "OBS|gpu|absf|0|0x00000000|0x00000000\n\
             OBS|gpu|acos|1|0x40000000|0x7fc00000\n",
        );
        let cmp = compare(&golden, &fresh);
        assert_eq!(cmp.agreed, 1); // absf agrees once width is normalised
        assert_eq!(cmp.divergences.len(), 1); // the two NaNs still diverge
        assert_eq!(cmp.divergences[0].kernel, "acos");
        assert_eq!(cmp.divergences[0].golden.as_deref(), Some("0xffc00000"));
        assert_eq!(cmp.divergences[0].fresh.as_deref(), Some("0x7fc00000"));
    }

    #[test]
    fn the_device_provenance_is_captured() {
        let c =
            GpuCorpus::parse("OBS|gpudev|vulkan|AMD Radeon Graphics (RADV GFX1033)|integrated\n");
        assert!(c.device.contains("gfx1033") || c.device.contains("GFX1033"));
    }
}
