# D116 - A reference oracle for GPU results

**Status:** decided
**Date:** 2026-09-26

`obscene-tool gpuref` recomputes each kernel on the CPU in `f64` through the host `libm`, rounds to
`f32`, and emits a corpus labelled as a reference. Exact operations must match it bit for bit;
transcendentals are measured against it. Kernels it cannot judge (f16 packing) are skipped and
counted. The reference stays IEEE-correct and is never taught a device's behaviour. `gpudiff` and
`gpustats` canonicalise hex fields and key lanes by their inputs.

**Why:** a diff between two runs says only that they differ; a reference says where a device is
approximate and where it is exact. A reference adjusted to match a device would stop flagging the
behaviour worth finding.

**Rejected:** device-against-device diffs only. Fabricating a baseline for kernels the reference
cannot judge.
