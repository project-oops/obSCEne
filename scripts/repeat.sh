#!/bin/sh
# Runs the module many times and counts how often an outcome occurs.
#
# # Why this exists
#
# Some faults are intermittent, and an intermittent fault needs a rate before it can be
# reported. "Crashes above forty live threads" is a bug report; "crashes sometimes" is a
# complaint, and the difference is arithmetic somebody has to do.
#
# It also protects against the opposite mistake, which this project has already made once.
# Four runs at three churn values produced no crashes and were nearly written up as
# evidence of a threshold. At a rate near one in seven, four runs miss it more often than
# they find it - twelve runs with zero events says almost nothing, and reporting it as a
# result would have been worse than not running it.
#
# So this reports the denominator as loudly as the numerator, and refuses to be quiet
# about a small sample.
#
#   sh scripts/repeat.sh --runs 20
#   sh scripts/repeat.sh --runs 20 --churn 8
#
# Each run is a fresh process. The module is rebuilt only when --churn is given, since
# rebuilding between runs would measure the build rather than the platform.
# Paths are derived from this script's own location rather than hardcoded, so the
# collection works wherever it is cloned. `$OOPS` is the parent holding all four projects.
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
OOPS="$(cd "$REPO/.." && pwd)"
EMULATORS="${EMULATORS:-$(cd "$OOPS/.." && pwd)/emulators}"

set -e

RUNS="${RUNS:-10}"
CHURN=""
EMULATOR="${EMULATOR:-$EMULATORS/shadps4/shadPS4.exe}"
VM="${VM:-wsl}"   # kept: call sites still pass it positionally
GENERATION="${GENERATION:-4}"

while [ $# -gt 0 ]; do
    case "$1" in
        --runs) RUNS="$2"; shift 2 ;;
        --churn) CHURN="$2"; shift 2 ;;
        --emulator) EMULATOR="$2"; shift 2 ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

here=$(dirname "$0")
# The build environment lives in scripts/wsl.sh, which replaced multipass. (D199)
. "$(dirname "$0")/wsl.sh"

if [ -n "$CHURN" ]; then
    echo "building with CHURN=$CHURN"
    vm exec "$VM" --working-directory "$VM_REPO" -- bash -lc \
        "GEN=$GENERATION CHURN=$CHURN sh scripts/sweep-build.sh > /tmp/sb.log 2>&1" >/dev/null
fi

work="${TMPDIR:-/tmp}/obscene-repeat.$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

complete=0
short=0
crashed=0
run=1
while [ "$run" -le "$RUNS" ]; do
    out="$work/run-$run.txt"
    sh "$here/run-emulator.sh" --emulator "$EMULATOR" --out "$out" >/dev/null 2>&1 || true

    if grep -q '^OBS|end' "$out" 2>/dev/null; then
        complete=$((complete + 1))
        printf '.'
    else
        short=$((short + 1))
        # A `try` with no `res` names the call that did not return - the whole point of
        # announce-before-attempting, and what turns "it crashed" into a bug report.
        where=$(awk -F'|' '
            $2 == "try" { open[$3] = 1 }
            $2 == "res" { delete open[$3] }
            END { for (id in open) { print id; break } }
        ' "$out")
        if [ -n "$where" ]; then
            crashed=$((crashed + 1))
            printf '\n  run %s died in %s\n' "$run" "$where"
        else
            printf '\n  run %s ended short with no dangling try\n' "$run"
        fi
    fi
    run=$((run + 1))
done

echo
echo "runs      : $RUNS${CHURN:+ at CHURN=$CHURN}"
echo "complete  : $complete"
echo "short     : $short"
echo "in a call : $crashed"

# The sample-size warning, stated in the output rather than left to the reader. A run of
# ten that finds nothing is consistent with a one-in-seven fault about a fifth of the
# time, which is not a result.
if [ "$crashed" -eq 0 ] && [ "$RUNS" -lt 20 ]; then
    echo
    echo "NOTE: $RUNS runs with no crash is not evidence of absence. A fault at one in"
    echo "      seven survives $RUNS runs often enough to matter; twenty is the minimum"
    echo "      worth drawing a conclusion from."
fi
