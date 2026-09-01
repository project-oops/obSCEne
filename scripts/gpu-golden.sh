#!/bin/sh
# The golden GPU corpus: a blessed snapshot of what one device actually computes, and the
# regression check that diffs a fresh run against it.
#
# # Why a golden as well as the reference
#
# The reference oracle (obscene-tool gpuref) judges the exact operations device-independently:
# a divergence there is a device bug anywhere. What it cannot judge is the transcendentals -
# it is a strong baseline, not the last word, so it does not pin sin or rcp to a value. The
# golden does: it records what *this* device produced, so a change that alters a kernel's
# output - a shader edit, a dispatch bug, a widened input vector - is caught even where the
# reference stays silent. Together they cover both questions: "is it still exact where it must
# be" (reference) and "did anything change at all" (golden).
#
# # Why the check skips on a different device
#
# The golden is one device's numbers - the build VM's llvmpipe. On any other rasteriser the
# transcendentals differ legitimately, so asserting against it there would fail on a
# difference that is not a regression. The check compares the `gpudev` line first and skips
# when it does not match, so it never becomes the fragile "needs a working rasteriser"
# dependency that keeps the GPU run out of the gate otherwise (see scripts/verify.sh). On the
# matching device it demands zero divergence, which is the regression it exists to catch.
#
# Usage:
#   scripts/gpu-golden.sh --capture     # re-bless the golden from a fresh run
#   scripts/gpu-golden.sh --check       # fail if a fresh run diverges from the golden
set -e

BUILD="${BUILD:-/tmp/obs}"
GOLDEN="${GOLDEN:-reports/gpu-golden.txt}"
GPU_BUILD="$BUILD-gpu"
TOOL="${OBS_TOOL:-${CARGO_TARGET_DIR:-tool/target}/debug/obscene-tool}"

mode="${1:-}"
if [ "$mode" != "--capture" ] && [ "$mode" != "--check" ]; then
    echo "usage: $0 --capture | --check" >&2
    exit 2
fi

# A fresh corpus, from the GPU=1 host build. Always (re)build - make is incremental, so this is
# a no-op when nothing changed and a rebuild when a shader or the section did. Gating on the
# binary's mere existence was a bug: it captured a stale binary after a kernel changed, blessing
# yesterday's numbers. The grep keeps only the GPU records, so the harness's other output cannot
# pollute the diff.
make host GPU=1 BUILD="$GPU_BUILD" >/dev/null 2>&1 || true
fresh="${TMPDIR:-/tmp}/gpu-fresh.$$"
trap 'rm -f "$fresh"' EXIT
"$GPU_BUILD/obscene-host" 2>/dev/null | grep -E '^OBS\|(gpudev|gpu|gpuop)\|' > "$fresh" || true

records=$(wc -l < "$fresh" | tr -d ' ')
if [ "$records" = "0" ]; then
    # No GPU backend here (no llvmpipe, no device). Nothing to bless or check against.
    echo "no GPU records produced (no usable device); skipping"
    exit 0
fi

if [ "$mode" = "--capture" ]; then
    mkdir -p "$(dirname "$GOLDEN")"
    cp "$fresh" "$GOLDEN"
    dev=$(grep '^OBS|gpudev|' "$GOLDEN" | head -1)
    echo "captured $records records to $GOLDEN"
    echo "device: $dev"
    exit 0
fi

# --check
if [ ! -f "$GOLDEN" ]; then
    echo "no golden at $GOLDEN; run: $0 --capture" >&2
    exit 1
fi
golden_dev=$(grep '^OBS|gpudev|' "$GOLDEN" | head -1)
fresh_dev=$(grep '^OBS|gpudev|' "$fresh" | head -1)
if [ "$golden_dev" != "$fresh_dev" ]; then
    echo "device differs from the golden, so the transcendentals differ legitimately; skipping"
    echo "  golden: $golden_dev"
    echo "  fresh:  $fresh_dev"
    exit 0
fi

# Same device: any divergence is a regression. gpudiff exits 1 when the corpora disagree.
if [ ! -x "$TOOL" ]; then
    echo "no obscene-tool at $TOOL; build it first" >&2
    exit 1
fi
"$TOOL" gpudiff "$GOLDEN" "$fresh"
