#!/bin/sh
# The hardware-day analysis, in one command: given a device GPU corpus, produce its reference,
# the exact-match diff, and the ULP approximation ranking.
#
# # Why this is one script
#
# The three tools answer three questions and are meant to be read together: `gpuref` says what
# each kernel *should* compute, `gpudiff` says which lanes are not bit-exact (the answer the
# exact operations owe), and `gpustats` says by how many ULP the rest are off (the answer the
# transcendentals owe). Run by hand they are three commands and an intermediate file; run here
# they are one command over a corpus. That corpus is llvmpipe today and a Steam Deck or a PS5
# the day one exists - the analysis does not change, only what produced the corpus does.
#
# # What it does NOT do
#
# It does not judge. `gpudiff` exiting non-zero on a transcendental is not a failure here - the
# transcendentals are *allowed* to differ from the correctly-rounded reference, which is the
# whole reason `gpustats` exists to measure the distance rather than gate on it. The golden
# regression gate (scripts/gpu-golden.sh) is the thing that fails a build; this is for reading.
set -e

CORPUS="${1:-}"
if [ -z "$CORPUS" ] || [ ! -f "$CORPUS" ]; then
    echo "usage: $0 <device-corpus>" >&2
    echo "  e.g. $0 reports/gpu-golden.txt" >&2
    exit 2
fi

TOOL="${OBS_TOOL:-${CARGO_TARGET_DIR:-tool/target}/debug/obscene-tool}"
if [ ! -x "$TOOL" ]; then
    echo "no obscene-tool at $TOOL - build it (cargo build in tool/) or set OBS_TOOL" >&2
    exit 1
fi

ref="${TMPDIR:-/tmp}/gpu-analyze-ref.$$"
trap 'rm -f "$ref"' EXIT

# The reference for exactly this corpus's inputs, kept in a temp file the two diffs share.
"$TOOL" gpuref "$CORPUS" > "$ref"

echo "=== exact-match diff: device vs reference ==="
echo "(the exact operations must be empty here; a transcendental listed here is not a failure,"
echo " only a lane the ULP table below then quantifies)"
# `|| true`: gpudiff exits 1 on any divergence, which is expected for the transcendentals, and
# a non-zero exit must not stop the script before the ULP ranking, which is the point.
"$TOOL" gpudiff "$CORPUS" "$ref" || true

echo
echo "=== ULP approximation ranking: device vs reference ==="
"$TOOL" gpustats "$CORPUS" "$ref"
