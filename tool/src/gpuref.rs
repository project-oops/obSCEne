//! A reference oracle for the GPU kernels: what each one would compute, near-exactly.
//!
//! # Why a reference makes the diff mean something
//!
//! `gpudiff` on its own compares run against run. What it cannot say is *which instructions
//! are approximate* - and that is the question that matters, because an emulator can compute
//! an exact `1.0/x` and real hardware cannot, so the instructions where hardware departs from
//! exact are exactly the ones an emulator must stop computing exactly.
//!
//! This computes each kernel's result on the CPU in double precision and rounds to f32 - a
//! high-precision baseline - and emits it as a corpus in the same format. `gpudiff
//! device-corpus reference` then shows, per operand, where the device is approximate. Run it
//! against llvmpipe today and it is a real finding; run it against a Deck's corpus later and
//! it is the RDNA2 approximation map.
//!
//! # What "reference" does and does not mean
//!
//! It is the host's `libm` taken through `f64`, not a proof of correct rounding. For the
//! exact operations - doubling, min, max, division, the roundings, fma - it *is* exact, and
//! the device must match it bit for bit; a divergence there is a device bug, not an
//! approximation. For the transcendentals it is a strong baseline against which the device's
//! approximation is measured, not a claim that libm is itself the last word. The emitted
//! `gpudev` line says `reference`, never a device, so it is never mistaken for silicon.
//!
//! Some kernels have no reference here and are skipped: `half` and `packhalf`, both of which
//! turn on an f16 rounding this build does not carry, and any kernel added later that this
//! table does not know. A skipped kernel simply does not appear in the reference, so `gpudiff`
//! reports it as device-only rather than inventing a baseline for it.

use std::fmt::Write as _;

use crate::gpudiff::GpuCorpus;

/// frexp for an f32: the `(mantissa, exponent)` with `x == mantissa * 2^exponent` and the
/// mantissa in [0.5, 1) - or `None` for the inf/NaN inputs GLSL frexp leaves undefined.
///
/// The mantissa is exact: scaling by a power of two shifts the exponent without touching the
/// significand, so `m as f32` loses nothing, and both halves are a bit-for-bit reference on
/// normal inputs. This is the IEEE-correct frexp, including full normalisation of subnormals -
/// so where a device does not normalise them (llvmpipe reports exponent -126, not the true
/// normalised value), the diff surfaces that denormal-handling difference rather than the
/// reference hiding it. The while-loops correct the one place `log2().floor()` can round the
/// wrong way, at a power-of-two boundary.
#[allow(
    clippy::cast_possible_truncation,
    clippy::float_arithmetic,
    clippy::arithmetic_side_effects,
    reason = "exponent extraction and power-of-two scaling: the casts and arithmetic are the \
              operation, and the scaling is exact so no precision is lost"
)]
fn frexpf(x: f32) -> Option<(f32, i32)> {
    if !x.is_finite() {
        return None; // inf/NaN: GLSL frexp is undefined here, so no reference
    }
    if x == 0.0 {
        return Some((x, 0)); // preserves the signed zero; e is 0 by definition
    }
    let xd = f64::from(x);
    let mut e = xd.abs().log2().floor() as i32 + 1;
    let mut m = xd / 2f64.powi(e);
    while m.abs() >= 1.0 {
        m /= 2.0;
        e += 1;
    }
    while m.abs() < 0.5 {
        m *= 2.0;
        e -= 1;
    }
    Some((m as f32, e))
}

/// Decodes the low 16 bits of `h` from f16 to f32, exactly.
///
/// f16 -> f32 loses nothing - every half is representable as a float - so this is a reference
/// the device must match bit for bit, unlike packing *to* f16 (which needs a rounding this build
/// does not judge). Normals shift the exponent by the bias difference and the mantissa left 13;
/// subnormals are `mant * 2^-24`, which is an exact normal f32; inf/NaN carry the mantissa up.
#[allow(
    clippy::cast_precision_loss,
    clippy::float_arithmetic,
    clippy::arithmetic_side_effects,
    reason = "bit assembly and an exact power-of-two scale for the subnormal case"
)]
fn f16_to_f32(h: u32) -> f32 {
    let h = h & 0xffff;
    let sign = (h & 0x8000) << 16;
    let exp = (h >> 10) & 0x1f;
    let mant = h & 0x3ff;
    if exp == 0 {
        if mant == 0 {
            return f32::from_bits(sign); // signed zero
        }
        // subnormal half: value is mant * 2^-24, exact as an f32
        let v = f32::from(mant as u16) * 2.0f32.powi(-24);
        return if sign != 0 { -v } else { v };
    }
    if exp == 0x1f {
        // inf (mant 0) or NaN (mant nonzero), mantissa carried into the f32 field
        return f32::from_bits(sign | 0x7f80_0000 | (mant << 13));
    }
    // normal: f32 exponent is the f16 exponent re-biased (- 15 + 127 = + 112)
    f32::from_bits(sign | ((exp + 112) << 23) | (mant << 13))
}

/// The near-exact result of `kernel` for the given inputs, or `None` if this build has no
/// reference for it (the operands are floats already decoded from their bit patterns).
///
/// Doubles are the working type for the transcendentals so the f32 result is correctly
/// rounded from a higher-precision intermediate. The exact operations are computed directly.
#[allow(
    clippy::cast_precision_loss,
    clippy::cast_possible_truncation,
    clippy::cast_possible_wrap,
    clippy::cast_lossless,
    clippy::cast_sign_loss,
    clippy::float_arithmetic,
    clippy::arithmetic_side_effects,
    reason = "this function is deliberate float and int conversion - the casts and the \
              arithmetic are its whole job, and each is the operation the kernel names; the \
              signed<->unsigned casts are the intBitsToFloat / snorm sign reinterpretations"
)]
fn reference(kernel: &str, inputs: &[f32]) -> Option<f32> {
    let a = inputs.first().copied()?;
    let b = || inputs.get(1).copied();
    let c = || inputs.get(2).copied();
    Some(match kernel {
        // Exact: the device must match these bit for bit.
        "double" => a * 2.0,
        // Halving is exact, so the reference is the true IEEE result including the denormal it
        // produces from the smallest normal; a device that flushes diverges, which is the
        // finding, not an approximation.
        "ftz" => (f64::from(a) * 0.5) as f32,
        "floor" => a.floor(),
        "ceil" => a.ceil(),
        "trunc" => a.trunc(),
        // GLSL round is implementation-defined at the half; roundEven is ties-to-even. Both
        // use ties-to-even here, which is what the common hardware does - a divergence on
        // `round` is therefore an observation about the device's choice, not a defect.
        "round" | "roundeven" => a.round_ties_even(),
        // GLSL fract is x - floor(x), which differs from Rust's f32::fract for negatives.
        "fract" => a - a.floor(),
        "absf" => a.abs(),
        // GLSL sign: -1/0/+1, with zero (and, by this form, NaN) mapping to 0 - not Rust's
        // signum, which returns +/-1 for zero and NaN for NaN.
        "signf" => {
            if a > 0.0 {
                1.0
            } else if a < 0.0 {
                -1.0
            } else {
                0.0
            }
        }
        // Rust float-to-int saturates (NaN to 0, out-of-range to the bound); GLSL int(x) is
        // undefined out of range, so a divergence in that region is expected, not a bug.
        "cvti" => (a as i32) as f32,
        // Float decomposition, exact. frexp splits x into a mantissa in [0.5,1) and an integer
        // exponent. The exponent is an int reinterpreted into the float slot, matching the
        // kernel's intBitsToFloat; both return early because frexpf may decline (inf/NaN).
        "frexpmant" => return frexpf(a).map(|(m, _e)| m),
        "frexpexp" => return frexpf(a).map(|(_m, e)| f32::from_bits(e as u32)),
        // Integer/bit ops on the input's 32-bit pattern, exact. The operand is read as bits
        // (a.to_bits() round-trips whatever the corpus recorded, NaN patterns included), and
        // the result is an int reinterpreted, matching the kernel's intBitsToFloat: a count or
        // position of 0..32, or the all-ones word for the "no bit set" answer -1.
        "bitcount" => return Some(f32::from_bits(a.to_bits().count_ones())),
        "findmsb" => {
            let bits = a.to_bits();
            // ilog2 is the highest set bit's index for nonzero; -1 (all ones) for the zero word.
            let msb = if bits == 0 { u32::MAX } else { bits.ilog2() };
            return Some(f32::from_bits(msb));
        }
        "findlsb" => {
            let bits = a.to_bits();
            let lsb = if bits == 0 {
                u32::MAX
            } else {
                bits.trailing_zeros()
            };
            return Some(f32::from_bits(lsb));
        }
        "bitreverse" => return Some(f32::from_bits(a.to_bits().reverse_bits())),
        // Bitfield extract/insert with the fixed field [8, 16) the kernels bake in. Extract is
        // a shift-and-mask; insert clears the field in the base and drops the operand's low
        // eight bits into it. Both exact.
        "bfe" => return Some(f32::from_bits((a.to_bits() >> 8) & 0xff)),
        "bfi" => {
            let base = a.to_bits();
            let insert = b()?.to_bits();
            let mask = 0xff00u32;
            return Some(f32::from_bits(
                (base & !mask) | (insert.wrapping_shl(8) & mask),
            ));
        }
        // Packing conversions: clamp, scale to a 16-bit integer, pack two into a word (first in
        // the low half). Computed in f32 to mirror the device's intermediate; the rounding at a
        // half-integer is the device's to choose, so a tie could diverge - a finding, not a bug.
        "packunorm" => {
            let pack = |v: f32| ((v.clamp(0.0, 1.0) * 65535.0).round_ties_even() as u32) & 0xffff;
            return Some(f32::from_bits(pack(a) | (pack(b()?) << 16)));
        }
        "packsnorm" => {
            let pack = |v: f32| {
                (((v.clamp(-1.0, 1.0) * 32767.0).round_ties_even() as i32) as u32) & 0xffff
            };
            return Some(f32::from_bits(pack(a) | (pack(b()?) << 16)));
        }
        // Unpacking the low half: unorm is the low 16 bits over 65535; snorm is the signed low
        // 16 bits over 32767, clamped (only -32768/32767 falls below -1); half is an exact
        // f16->f32 decode. All exact - the device must match bit for bit.
        "unpackunorm" => return Some(f32::from(a.to_bits() as u16) / 65535.0),
        "unpacksnorm" => {
            let raw = f32::from(a.to_bits() as u16 as i16);
            return Some((raw / 32767.0).clamp(-1.0, 1.0));
        }
        "unpackhalf" => return Some(f16_to_f32(a.to_bits())),
        // divf and its relaxed twin share the correctly-rounded reference: the point is that
        // on hardware divrelaxed (fast v_fdiv) diverges from it while divf (the IEEE sequence)
        // matches. IEEE division is correctly rounded in f32.
        "divf" | "divrelaxed" => a / b()?,
        // GLSL min/max as the spec's select form (y<x?y:x), so NaN propagation is defined
        // here - the device's own choice is what the diff then reveals.
        "minf" => {
            let y = b()?;
            if y < a { y } else { a }
        }
        "maxf" => {
            let y = b()?;
            if y > a { y } else { a }
        }
        // Fused: the product is not rounded before the add. mul_add is the fused result, the
        // whole point against a two-step a*b then +c.
        "fmaf" => a.mul_add(b()?, c()?),
        // Transcendentals: f64 intermediate, rounded to f32. The relaxed twins (rcprelaxed,
        // sqrtrelaxed) share the correctly-rounded reference with their full forms - on hardware
        // the relaxed one takes the bare SFU op and diverges from it, which is the finding.
        "rcp" | "rcprelaxed" => (1.0f64 / f64::from(a)) as f32,
        "rsq" => (1.0f64 / f64::from(a).sqrt()) as f32,
        "sqrt" | "sqrtrelaxed" => f64::from(a).sqrt() as f32,
        "sin" => f64::from(a).sin() as f32,
        "cos" => f64::from(a).cos() as f32,
        "tan" => f64::from(a).tan() as f32,
        "asin" => f64::from(a).asin() as f32,
        "acos" => f64::from(a).acos() as f32,
        "atan" => f64::from(a).atan() as f32,
        "sinh" => f64::from(a).sinh() as f32,
        "cosh" => f64::from(a).cosh() as f32,
        "tanh" => f64::from(a).tanh() as f32,
        "exp" => f64::from(a).exp() as f32,
        "ln" => f64::from(a).ln() as f32,
        "exp2" => f64::from(a).exp2() as f32,
        "log2" => f64::from(a).log2() as f32,
        "powf" => f64::from(a).powf(f64::from(b()?)) as f32,
        // No reference (half's f16 round-trip, or a kernel this table does not know).
        _ => return None,
    })
}

fn hex_to_f32(hex: &str) -> Option<f32> {
    let digits = hex.strip_prefix("0x").unwrap_or(hex);
    u32::from_str_radix(digits, 16).ok().map(f32::from_bits)
}

/// Renders a reference corpus from the inputs found in `corpus`.
///
/// Every lane whose kernel has a reference is re-emitted with the reference output, in the
/// same `gpu`/`gpuop` shape (arity decides which), so the result diffs against a device
/// corpus with the existing `gpudiff`. Lanes with no reference are dropped, and a count of
/// them is returned so the caller can say what was skipped rather than leaving it silent.
pub fn render_reference(corpus: &GpuCorpus) -> (String, usize) {
    let mut out = String::new();
    out.push_str("OBS|gpudev|reference|host libm via f64|reference\n");
    let mut skipped: usize = 0;
    let mut lane: u64 = 0;
    for (kernel, inputs) in corpus.lanes.keys() {
        let values: Option<Vec<f32>> = inputs.iter().map(|h| hex_to_f32(h)).collect();
        let Some(values) = values else {
            skipped = skipped.saturating_add(1);
            continue;
        };
        let Some(result) = reference(kernel, &values) else {
            skipped = skipped.saturating_add(1);
            continue;
        };
        let bits = result.to_bits();
        // One input is a unary lane (the `gpu` shape); more is multi-operand (`gpuop`,
        // output first then inputs).
        if let [only] = inputs.as_slice() {
            let _ = writeln!(out, "OBS|gpu|{kernel}|{lane}|{only}|0x{bits:08x}");
        } else {
            let _ = write!(out, "OBS|gpuop|{kernel}|{lane}|0x{bits:08x}");
            for input in inputs {
                let _ = write!(out, "|{input}");
            }
            out.push('\n');
        }
        lane = lane.saturating_add(1);
    }
    (out, skipped)
}

#[cfg(test)]
#[allow(
    clippy::indexing_slicing,
    reason = "test fixtures build known-size inputs; a panic here is the failure signal"
)]
mod tests {
    use super::*;

    fn bits(x: f32) -> u32 {
        x.to_bits()
    }

    /// The exact operations must be exact - this is what proves the reference itself is
    /// right, because the device is expected to match these bit for bit.
    #[test]
    fn exact_operations_are_exact() {
        assert_eq!(reference("double", &[3.0]), Some(6.0));
        assert_eq!(reference("divf", &[1.0, 2.0]), Some(0.5));
        assert_eq!(reference("fmaf", &[2.0, 3.0, 1.0]), Some(7.0));
        assert_eq!(reference("floor", &[2.7]), Some(2.0));
    }

    /// GLSL semantics that differ from Rust's defaults are the ones easiest to get wrong.
    #[test]
    fn glsl_semantics_not_rust_defaults() {
        // fract of a negative is x - floor(x), not Rust's x - trunc(x).
        assert_eq!(reference("fract", &[-0.25]), Some(0.75));
        // sign of zero is 0, not Rust signum's 1.0.
        assert_eq!(reference("signf", &[0.0]), Some(0.0));
        // min's select form returns the non-NaN operand's counterpart per the spec.
        assert_eq!(reference("minf", &[5.0, f32::NAN]), Some(5.0));
    }

    #[test]
    fn a_transcendental_rounds_from_f64() {
        // 1/3 correctly rounded to f32 is 0x3eaaaaab.
        assert_eq!(reference("rcp", &[3.0]).map(bits), Some(0x3eaa_aaab));
    }

    /// frexp is exact - mantissa in [0.5,1), integer exponent - and the exponent comes back
    /// as its int bits reinterpreted, matching the kernel's intBitsToFloat. inf/NaN decline.
    #[test]
    fn frexp_splits_exactly() {
        // 3 = 0.75 * 2^2
        assert_eq!(reference("frexpmant", &[3.0]), Some(0.75));
        assert_eq!(reference("frexpexp", &[3.0]).map(bits), Some(2));
        // 1 = 0.5 * 2^1
        assert_eq!(reference("frexpmant", &[1.0]), Some(0.5));
        assert_eq!(reference("frexpexp", &[1.0]).map(bits), Some(1));
        // 0 -> (0, 0), signed zero preserved
        assert_eq!(reference("frexpmant", &[0.0]), Some(0.0));
        assert_eq!(reference("frexpexp", &[0.0]).map(bits), Some(0));
        // a value below 0.5 has a negative exponent, reinterpreted as its two's-complement
        // bits: 0.25 = 0.5 * 2^-1, and intBitsToFloat(-1) is the all-ones word.
        assert_eq!(reference("frexpexp", &[0.25]).map(bits), Some(0xffff_ffff));
        // inf/NaN are undefined for GLSL frexp and are skipped, not asserted
        assert_eq!(reference("frexpmant", &[f32::INFINITY]), None);
        assert_eq!(reference("frexpexp", &[f32::NAN]), None);
    }

    /// Integer/bit ops read the input as its bit pattern and return an int, exact. The -1 that
    /// findMSB/findLSB give for a zero word comes back as the all-ones word, matching
    /// intBitsToFloat(-1).
    #[test]
    fn bit_ops_operate_on_the_bit_pattern() {
        let of = f32::from_bits;
        // 0b111: three bits set, msb at position 2, lsb at 0
        assert_eq!(reference("bitcount", &[of(0b111)]).map(bits), Some(3));
        assert_eq!(reference("findmsb", &[of(0b111)]).map(bits), Some(2));
        assert_eq!(reference("findlsb", &[of(0b110)]).map(bits), Some(1));
        // the zero word: count 0, msb and lsb both -1
        assert_eq!(reference("bitcount", &[of(0)]).map(bits), Some(0));
        assert_eq!(reference("findmsb", &[of(0)]).map(bits), Some(0xffff_ffff));
        assert_eq!(reference("findlsb", &[of(0)]).map(bits), Some(0xffff_ffff));
        // reverse sends bit 0 to bit 31
        assert_eq!(
            reference("bitreverse", &[of(1)]).map(bits),
            Some(0x8000_0000)
        );
        // bitfield extract of the fixed [8,16) field
        assert_eq!(reference("bfe", &[of(0x0000_ab00)]).map(bits), Some(0xab));
        // bitfield insert drops the operand's low 8 bits into base's [8,16) field
        assert_eq!(
            reference("bfi", &[of(0), of(0x12cd)]).map(bits),
            Some(0x0000_cd00)
        );
    }

    /// The FTZ probe is an exact halving; the reference is the true IEEE result, so a flushing
    /// device diverges from it rather than the reference hiding the flush.
    #[test]
    fn ftz_is_exact_halving() {
        assert_eq!(reference("ftz", &[2.0]), Some(1.0));
        // the smallest normal halves to a representable denormal, not zero
        let smallest_normal = f32::from_bits(0x0080_0000);
        assert_eq!(
            reference("ftz", &[smallest_normal]).map(bits),
            Some(0x0040_0000)
        );
    }

    /// Packing clamps/scales/packs (first operand in the low half); unpacking reverses the low
    /// half; f16 -> f32 is exact.
    #[test]
    fn pack_unpack_conversions() {
        let of = f32::from_bits;
        // packUnorm: 0.0 -> 0x0000, 1.0 -> 0xffff; the second operand is the high half
        assert_eq!(
            reference("packunorm", &[0.0, 1.0]).map(bits),
            Some(0xffff_0000)
        );
        assert_eq!(
            reference("packunorm", &[1.0, 0.0]).map(bits),
            Some(0x0000_ffff)
        );
        // packSnorm: 1.0 -> 0x7fff, -1.0 -> 0x8001 (i16 -32767)
        assert_eq!(
            reference("packsnorm", &[1.0, -1.0]).map(bits),
            Some(0x8001_7fff)
        );
        // unpackUnorm low half: 0xffff -> 1.0, 0 -> 0.0
        assert_eq!(reference("unpackunorm", &[of(0x0000_ffff)]), Some(1.0));
        assert_eq!(reference("unpackunorm", &[of(0)]), Some(0.0));
        // unpackSnorm low half: 0x7fff -> 1.0, 0x8001 -> -1.0
        assert_eq!(reference("unpacksnorm", &[of(0x0000_7fff)]), Some(1.0));
        assert_eq!(reference("unpacksnorm", &[of(0x0000_8001)]), Some(-1.0));
        // unpackHalf low half: f16 1.0 (0x3c00) -> 1.0, f16 -2.0 (0xc000) -> -2.0
        assert_eq!(reference("unpackhalf", &[of(0x0000_3c00)]), Some(1.0));
        assert_eq!(reference("unpackhalf", &[of(0x0000_c000)]), Some(-2.0));
    }

    #[test]
    fn a_kernel_with_no_reference_is_none() {
        assert_eq!(reference("half", &[1.0]), None);
        assert_eq!(reference("nonesuch", &[1.0]), None);
    }

    #[test]
    fn the_reference_corpus_round_trips_the_format() {
        let device = GpuCorpus::parse(
            "OBS|gpu|rcp|0|0x40400000|0xdeadbeef\n\
             OBS|gpuop|fmaf|0|0xdeadbeef|0x40000000|0x40400000|0x3f800000\n\
             OBS|gpu|half|0|0x3f800000|0x3f800000\n",
        );
        let (text, skipped) = render_reference(&device);
        assert_eq!(skipped, 1); // half has no reference
        let reref = GpuCorpus::parse(&text);
        // rcp(3) reference is the correctly-rounded 1/3, not the device's 0xdeadbeef.
        let key = ("rcp".to_string(), vec!["0x40400000".to_string()]);
        assert_eq!(
            reref.lanes.get(&key).map(String::as_str),
            Some("0x3eaaaaab")
        );
        assert_eq!(reref.device, "reference|host libm via f64|reference");
    }
}
