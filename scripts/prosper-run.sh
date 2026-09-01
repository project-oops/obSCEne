#!/bin/bash
# Run obSCEne under prosper, headless, until the resume mechanism stops making progress.
#
# The sixth loader, and the only one with no window. Every other loader in the sweep opens one
# on the user's desktop, which is a real cost: a run long enough to be minimised is a run whose
# screenshot fails, and the screenshot is the report for loaders whose text channel does not
# work. prosper has no such channel problem and no window, so it can be run as often as needed.
#
# Runs happen inside WSL. `prosper` links against a Linux build of its own core; there is no
# Windows binary to point at.
#
# Usage:  prosper-run.sh [--rounds N] [--fresh] [--gen 4|5]
set -e

ROUNDS=4
FRESH=no
GEN=5

while [ $# -gt 0 ]; do
    case "$1" in
        --rounds) ROUNDS="$2"; shift 2 ;;
        --fresh)  FRESH=yes; shift ;;
        --gen)    GEN="$2"; shift 2 ;;
        -h|--help) sed -n '2,14p' "$0"; exit 0 ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

d="$HOME/obs-prosper"
mkdir -p "$d"

# The report is the resume state, so it is preserved between rounds by default. (D172/D181)
#
# Deleting it is what `--fresh` is for, and it is rarely what you want: obSCEne reads the
# previous report at startup, finds the check that announced itself and never returned, and
# skips it. Wiping the directory each round - which the first throwaway version of this script
# did - makes every round stop in the same place forever and look like a loader that cannot
# get past one check, rather than one that has not been asked twice.
if [ "$FRESH" = yes ]; then rm -f "$d/obscene-report.txt"; fi

cp "$HOME/obs-f$GEN/obscene.module.elf" "$d/eboot.bin"
cd "$d"

round=1
while [ "$round" -le "$ROUNDS" ]; do
    timeout 180 "$HOME/prosper-build/boot_trace" "$d" > "$d/round$round.log" 2>&1 || true
    # prosper's own stub reports interleave with ours in the same stream, which is the point:
    # this is the one loader that says which functions it has not implemented, so its log is
    # evidence about obSCEne's stub-detection and not only about obSCEne's progress.
    n=$(grep -c '^OBS|' "$d/round$round.log" || true)
    last=$(grep '^OBS|try' "$d/round$round.log" | tail -1 | cut -d'|' -f3)
    ended=$(grep -c '^OBS|end' "$d/round$round.log" || true)
    printf 'round %-3s %6s records   last=%-40s end=%s\n' "$round" "$n" "$last" "$ended"
    if [ "$ended" != "0" ]; then break; fi
    round=$((round + 1))
done

cp "$d/obscene-report.txt" "$d/final-report.txt" 2>/dev/null || true
echo "logs: \$HOME/obs-prosper/round*.log"
