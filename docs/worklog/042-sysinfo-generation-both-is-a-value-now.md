# `sysinfo|generation`: `both` is a value now, and the report value-sets are declared open


Two points came back from the consumer (orbistoun), both obSCEne's to settle.

**`both` and `neither` had collapsed to the same `unknown`.** The earlier fix sent `both` to
`unconfirmed|unknown` and `neither` to `absent|unknown` - technically distinct, but `unconfirmed`
means "resolves, no confirmed signature to read it" (IP/FW), which mislabels `both` (a *positive*
observation) as a flavour of "couldn't read it", and the value was identically `unknown`. The
005-section already treats the two as distinct partials; the record now matches. `both` is a
value of its own: `sysinfo|generation|known|both`, distinct from `neither`'s `absent|unknown`.
It still names no console - presence is not implementation - but "both driver families resolve"
is a real finding (the stub-everything fingerprint, or real back-compat) and a different fact
from an absence. Verified on shadPS4: `OBS|sysinfo|generation|known|both`, clean run.

**"Is the state vocabulary closed?" - decided and written into the contract.** It was
unspecified: OUTPUT.md's versioning covers new record kinds, appended fields and field
meaning/order, but was silent on new *values* within an enum. Decision: **report enum values are
open** (a reader tolerates an unrecognised value, obSCEne may append one without a version bump -
same reasoning as adding a record kind, because the report exists to surface findings); **the
protocol grammar is the closed exception** (verbs, refusal reasons, capabilities are fixed lists
the spec and `protocol.py` enforce). Landed in `docs/OUTPUT.md` and the handover.

### Process change worth keeping

- **A shared file replaces relaying by hand.** obSCEne and orbistoun were passing feedback
  by having messages copied between two Claude sessions by hand. That is now `<shared>\obscene-orbistoun-bridge.md`
  - a neutral log (neither repo owns or commits it) both sides post to. Durable contracts still
  live in `OUTPUT.md` / `PROTOCOL.md` / the handover; the bridge is for the conversation. `make
  check` green (117) throughout.

---

Pack/unpack conversions + per-kernel input scheme (D146). The five D124 deferred: packunorm,
packsnorm (arity 2), unpackunorm, unpacksnorm, unpackhalf (unary, packed word). They needed
range-bound operands, so first built the input scheme: `obs_gpu_operands(name)` picks the
multi-operand set ([0,1] for packunorm, [-1,1] for packsnorm, bit edges otherwise) - the
multi-path twin of D124's unary bit-kernel predicate; unpack kernels join the bit-pattern list.
One predicate per path, no schema change. 50 kernels; all match the reference bit-for-bit.
Census: cvt_pknorm_u16/i16 moved isa -> covered (they were GLSL-reachable all along), and the
unpack conversions added to base ISA. unpackHalf HAS a reference (f16->f32 is exact via a
bit-exact f16_to_f32 decoder), unlike packHalf which needs f16 rounding. Golden inspected then
re-blessed: 133 new lanes, all five new kernels, nothing else moved. 1668 records. verify: ok.

---

gpustats: the diff as a distance (D148). `obscene-tool gpustats device reference` reports
per-kernel ULP, ranked worst first - the question the transcendentals need (how far, not just
whether). Analysis tool like gpuref (prints, exits 0, no verify gate; correctness in unit tests).
On the llvmpipe golden it named the shape: well-behaved SFU at 1-6 ULP (rsq 1, exp2 2, exp/cosh
6), catastrophic on large-argument trig (sin/tan/cos of ~1e20 land on the wrong side of range
reduction). Exact ops show 0.

### Surprise (two, both from the first output)
- The naive `0x80000000 - bits` ULP ordering put +0 and -0 2^32 apart, making min/max/pow/sinh
  look catastrophic on signed-zero lanes. They compare equal as numbers, so the mirror-magnitude
  ordering collapses them to 0 ULP; the sign of a zero is gpudiff's concern, not an error metric's.
- Integer-output kernels (cvti, bitcount, bfe, packs...) read as floats give nonsense distances
  (a bitcount of 32 as a float is a tiny denormal). Omitted by name, with a comment that a new
  integer kernel must be added to the list; they are gpudiff's exact-or-bug domain. 123 tool
  tests, clippy clean; C side untouched so verify not re-run for it.

