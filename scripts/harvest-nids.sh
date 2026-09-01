#!/bin/sh
# Pulls NID-to-name pairs out of an emulator's resolution log.
#
# An emulator that resolves imports by NID usually prints the name it matched. That makes
# its log a free corpus: every line is somebody else's implementation of the same hash
# agreeing - or not - with ours.
#
# They are worth two things:
#
#   * a lookup table, so unresolved NIDs turn into names without any guessing;
#   * a test set, so a candidate generator can be measured before it is believed.
#     `obscene-tool crack --known` reports how many of these a generator reproduces, and a
#     generator that cannot regenerate names already known is not evidence about names that
#     are not.
#
# Pairs where the emulator itself did not know the name are dropped: it prints those as
# "Unknown", and recording that would put a name in the table that is not one.
#
# Merges rather than replaces, since each run sees only the NIDs that run happened to
# import.
set -e

OUT="${OUT:-data/nid-corpus.txt}"

if [ $# -eq 0 ]; then
    echo "usage: sh scripts/harvest-nids.sh <log> [more logs...]" >&2
    echo "       OUT=path to write somewhere other than $OUT" >&2
    exit 2
fi

work="${TMPDIR:-/tmp}/obscene-harvest.$$"
trap 'rm -f "$work" "$work.pairs" "$work.header"' EXIT

# What is already known, and the provenance header, kept as written.
: > "$work.pairs"
: > "$work.header"
if [ -f "$OUT" ]; then
    grep -E '^\s*#|^\s*$' "$OUT" > "$work.header" || true
    grep -vE '^\s*#|^\s*$' "$OUT" >> "$work.pairs" || true
fi
before=$(sort -u "$work.pairs" | wc -l | tr -d ' ')

read=0
skipped=0
for log in "$@"; do
    if [ ! -r "$log" ]; then
        # A glob picks up whatever is there, including files this process cannot open. One
        # unreadable file is not a reason to abandon a harvest across dozens of others.
        skipped=$((skipped + 1))
        continue
    fi
    read=$((read + 1))
    sed -n 's/.*Stub resolved \([A-Za-z0-9+_-]*\) as \([A-Za-z0-9_]*\).*/\1 \2/p' "$log" \
        | grep -v ' Unknown$' >> "$work.pairs" || true
done

[ "$skipped" -gt 0 ] && echo "warning: $skipped log(s) could not be read" >&2

sort -u "$work.pairs" > "$work"
after=$(wc -l < "$work" | tr -d ' ')

if [ ! -s "$work.header" ]; then
    {
        echo "# Known NID to name pairs, harvested by scripts/harvest-nids.sh."
        echo "# See docs/WORKFLOW.md for where these fit and why they matter."
        echo "#"
    } > "$work.header"
fi

mkdir -p "$(dirname "$OUT")"
cat "$work.header" "$work" > "$OUT"

echo "$after pairs ($((after - before)) new) from $read log(s), written to $OUT"
