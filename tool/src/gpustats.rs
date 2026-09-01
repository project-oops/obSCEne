//! Per-kernel ULP statistics: how far each device output sits from the reference, quantified.
//!
//! # Why "diverged" is not enough
//!
//! `gpudiff` answers "does this lane match" - the yes/no the exact operations need, and the
//! regression gate. The transcendentals need more. `rcp` and `sin` are *allowed* to be
//! approximate, so the question is not whether they differ from the correctly-rounded reference
//! but by how much: one ULP is a good special-function unit, a hundred is a bug. This reduces a
//! corpus-against-reference diff to that number - per kernel, ranked worst first - the
//! approximation map as a table rather than a list of lanes.
//!
//! On llvmpipe today it names that rasteriser's transcendental error; on a Deck's corpus later
//! it is RDNA2's, and against an emulator's corpus it is "the emulator's sin is N ULPs off where
//! the hardware is M" - the shader gap, measured.
//!
//! # What a ULP is here
//!
//! The distance in representable floats between two values: adjacent floats are one ULP apart.
//! It is defined between two ordinary numbers and between two infinities; it is *not* defined
//! between a NaN and a number, so those lanes are counted apart as "incomparable" rather than
//! folded into a distance that would mean nothing.

use std::collections::BTreeMap;
use std::fmt::Write as _;

use crate::gpudiff::GpuCorpus;

/// Maps float bits to a total ordering where adjacent representable floats are adjacent
/// integers - so the difference of two such integers counts the floats between them.
///
/// Non-negatives keep their magnitude; negatives are mirrored to the same magnitude negated. The
/// point of doing it this way rather than the classic `0x80000000 - bits`: it collapses +0 and
/// -0 to the same point (both map to 0), so they read as zero ULP apart. They compare equal as
/// numbers, and the *sign* of a zero is a bit-level fact that gpudiff catches - not an
/// approximation error, which is all this tool measures.
#[allow(
    clippy::arithmetic_side_effects,
    reason = "the magnitude is 31 bits, so negating it stays well inside i64"
)]
fn ordered(bits: u32) -> i64 {
    let magnitude = i64::from(bits & 0x7fff_ffff);
    if bits & 0x8000_0000 != 0 {
        -magnitude
    } else {
        magnitude
    }
}

/// ULP distance between two float bit patterns, or `None` when exactly one is NaN - there is no
/// meaningful distance to a NaN. Two NaNs are zero apart: equally not-a-number.
#[allow(
    clippy::arithmetic_side_effects,
    reason = "ordered() bounds both operands to [0, 2^32], so the difference stays in i64"
)]
fn ulp(a: u32, b: u32) -> Option<u64> {
    let (fa, fb) = (f32::from_bits(a), f32::from_bits(b));
    match (fa.is_nan(), fb.is_nan()) {
        (true, true) => Some(0),
        (false, false) => Some((ordered(a) - ordered(b)).unsigned_abs()),
        _ => None,
    }
}

fn hex(s: &str) -> Option<u32> {
    u32::from_str_radix(s.strip_prefix("0x").unwrap_or(s), 16).ok()
}

/// Whether a kernel's output is an integer, a bitfield or a packed word rather than a float.
///
/// ULP is a distance between floats; for these it is meaningless - the output bits are an
/// integer count, a reversed word or two packed halves, and reading them as a float gives a
/// number nobody computed. gpudiff is the tool for those: their answer is exact-or-bug, not
/// near-or-far. Listed by name because the record carries no type tag; **a new integer-output
/// kernel must be added here**, or it will show a nonsense distance. Float-output kernels,
/// including the unpack conversions, are deliberately absent so they are measured.
fn is_integer_output(kernel: &str) -> bool {
    matches!(
        kernel,
        "cvti"
            | "bitcount"
            | "findmsb"
            | "findlsb"
            | "bitreverse"
            | "frexpexp"
            | "bfe"
            | "bfi"
            | "packunorm"
            | "packsnorm"
    )
}

/// The worst (largest-ULP) lane a kernel had, for the report's example.
pub struct Worst {
    pub inputs: Vec<String>,
    pub device: String,
    pub reference: String,
    pub ulp: u64,
}

/// One kernel's error against the reference.
pub struct KernelStat {
    pub kernel: String,
    /// Lanes compared (both sides present and both a number, or both NaN).
    pub compared: usize,
    /// Of those, exact (zero ULP).
    pub exact: usize,
    /// Of those, off by one or more ULP.
    pub diverged: usize,
    /// Lanes where one side was NaN and the other a number - no ULP distance.
    pub incomparable: usize,
    pub max_ulp: u64,
    pub sum_ulp: u128,
    pub worst: Option<Worst>,
}

/// Per-kernel ULP statistics over the lanes present in both corpora, worst kernel first.
///
/// Lanes only one side has are ignored here: gpudiff is the tool that reports coverage
/// differences, and folding them in would confuse "this kernel is approximate" with "these runs
/// probed different things".
#[allow(
    clippy::arithmetic_side_effects,
    reason = "the counters are lane tallies over a finite corpus; they cannot realistically \
              overflow usize/u128, and a saturating add would only hide a bug if they did"
)]
pub fn analyze(device: &GpuCorpus, reference: &GpuCorpus) -> Vec<KernelStat> {
    let mut by_kernel: BTreeMap<String, KernelStat> = BTreeMap::new();
    for (key, dev_out) in &device.lanes {
        if is_integer_output(&key.0) {
            continue; // ULP is not a distance between integers; gpudiff handles these
        }
        let Some(ref_out) = reference.lanes.get(key) else {
            continue;
        };
        let entry = by_kernel
            .entry(key.0.clone())
            .or_insert_with(|| KernelStat {
                kernel: key.0.clone(),
                compared: 0,
                exact: 0,
                diverged: 0,
                incomparable: 0,
                max_ulp: 0,
                sum_ulp: 0,
                worst: None,
            });
        let (Some(d), Some(r)) = (hex(dev_out), hex(ref_out)) else {
            continue; // a value field that is not hex - a log fragment; not this tool's job
        };
        match ulp(d, r) {
            None => entry.incomparable += 1,
            Some(0) => {
                entry.compared += 1;
                entry.exact += 1;
            }
            Some(u) => {
                entry.compared += 1;
                entry.diverged += 1;
                entry.sum_ulp += u128::from(u);
                if u > entry.max_ulp {
                    entry.max_ulp = u;
                    entry.worst = Some(Worst {
                        inputs: key.1.clone(),
                        device: dev_out.clone(),
                        reference: ref_out.clone(),
                        ulp: u,
                    });
                }
            }
        }
    }
    let mut stats: Vec<KernelStat> = by_kernel.into_values().collect();
    // Worst first, ties broken by name so the order is stable.
    stats.sort_by(|a, b| {
        b.max_ulp
            .cmp(&a.max_ulp)
            .then_with(|| a.kernel.cmp(&b.kernel))
    });
    stats
}

/// Renders the statistics as a table, worst kernel first.
#[allow(
    clippy::cast_precision_loss,
    reason = "the mean is a human-readable summary, not a value anything computes on; f64 \
              precision loss on a ULP tally is immaterial"
)]
pub fn render(stats: &[KernelStat], device: &str, reference: &str) -> String {
    let mut out = String::new();
    let name = |s: &str| {
        if s.is_empty() {
            "(none stated)".to_owned()
        } else {
            s.to_owned()
        }
    };
    let _ = writeln!(out, "device:    {}", name(device));
    let _ = writeln!(out, "reference: {}", name(reference));
    let _ = writeln!(
        out,
        "note: float-output kernels only; integer/bit/packed ones (cvti, bitcount, bfe, pack*, \
         ...) are omitted - use gpudiff for their exact match"
    );
    if device == reference && !device.is_empty() {
        let _ = writeln!(
            out,
            "note: both sides are the same device; the distances are nondeterminism, not error"
        );
    }
    let _ = writeln!(
        out,
        "{:<12} {:>6} {:>6} {:>8} {:>7} {:>8}  worst",
        "kernel", "lanes", "exact", "diverged", "max ulp", "mean ulp"
    );
    for s in stats {
        let mean = if s.compared > 0 {
            s.sum_ulp as f64 / s.compared as f64
        } else {
            0.0
        };
        let _ = write!(
            out,
            "{:<12} {:>6} {:>6} {:>8} {:>7} {:>8.2}",
            s.kernel, s.compared, s.exact, s.diverged, s.max_ulp, mean
        );
        if let Some(w) = &s.worst {
            let _ = write!(
                out,
                "  {}({}) dev={} ref={} ({} ulp)",
                s.kernel,
                w.inputs.join(","),
                w.device,
                w.reference,
                w.ulp
            );
        }
        if s.incomparable > 0 {
            let _ = write!(out, "  [{} incomparable]", s.incomparable);
        }
        out.push('\n');
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
    fn one_ulp_is_one_ulp() {
        // 0x3f800000 is 1.0; the next float up is 0x3f800001, one ULP away.
        assert_eq!(ulp(0x3f80_0000, 0x3f80_0001), Some(1));
        // identical bits are zero apart
        assert_eq!(ulp(0x3f80_0000, 0x3f80_0000), Some(0));
    }

    #[test]
    fn signed_zeros_are_equal_and_subnormals_straddle_them() {
        // +0 and -0 compare equal as numbers, so zero ULP apart - the sign of a zero is a
        // bit fact for gpudiff, not an approximation error.
        assert_eq!(ulp(0x0000_0000, 0x8000_0000), Some(0));
        // the smallest negative subnormal is one step below the (collapsed) zero
        assert_eq!(ulp(0x8000_0000, 0x8000_0001), Some(1));
        // the smallest positive and negative subnormals straddle zero: two steps apart
        assert_eq!(ulp(0x0000_0001, 0x8000_0001), Some(2));
    }

    #[test]
    fn nan_is_incomparable_to_a_number_but_not_to_nan() {
        assert_eq!(ulp(0x7fc0_0000, 0x3f80_0000), None);
        assert_eq!(ulp(0x7fc0_0000, 0x7fc0_0000), Some(0));
    }

    #[test]
    fn analyze_ranks_worst_kernel_first_and_counts() {
        // sin off by 2 ULP, rcp exact, on one lane each.
        let device = GpuCorpus::parse(
            "OBS|gpu|rcp|0|0x40400000|0x3eaaaaab\n\
             OBS|gpu|sin|0|0x3f800000|0x3f576aa6\n",
        );
        let reference = GpuCorpus::parse(
            "OBS|gpu|rcp|0|0x40400000|0x3eaaaaab\n\
             OBS|gpu|sin|0|0x3f800000|0x3f576aa4\n",
        );
        let stats = analyze(&device, &reference);
        assert_eq!(stats.len(), 2);
        // sin (2 ulp) ranks before rcp (0 ulp)
        assert_eq!(stats[0].kernel, "sin");
        assert_eq!(stats[0].max_ulp, 2);
        assert_eq!(stats[0].diverged, 1);
        assert_eq!(stats[1].kernel, "rcp");
        assert_eq!(stats[1].exact, 1);
        assert_eq!(stats[1].max_ulp, 0);
    }
}
