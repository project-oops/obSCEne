#!/bin/sh
# Runs the suite to completion, excluding whatever kills the process.
#
# A call that ends the process takes every check behind it with it. The first run of this
# suite against an emulator reached 110 records of a possible 513 for exactly that reason,
# and there was no way to see what was behind the crash without getting past it.
#
# So a complete sweep is iterative: run, see which call did not return, add it to the
# exclusion list, run again.
#
# The first pass is always made with an empty list, deliberately. A crash is a finding and
# excluding one is how you see what is behind it, not how you tidy it away. Excluded checks
# are reported as skips with a reason, so they stay visible, and `Skip` ranks below `Fail`
# in `diff` - burying a failure this way would read as a regression, which is what it is.
# Paths are derived from this script's own location rather than hardcoded, so the
# collection works wherever it is cloned. `$OOPS` is the parent holding all four projects.
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
OOPS="$(cd "$REPO/.." && pwd)"
EMULATORS="${EMULATORS:-$(cd "$OOPS/.." && pwd)/emulators}"

set -e

EMULATOR="${EMULATOR:-$EMULATORS/shadps4/shadPS4.exe}"
OUT="${OUT:-reports/full-sweep.txt}"
MAX_ROUNDS="${MAX_ROUNDS:-12}"
# Passed to the runner. The default suits shadPS4; a slower loader with a large census
# needs far more, and getting it wrong now reports itself rather than corrupting the
# exclusion list.
TIMEOUT="${TIMEOUT:-70}"
VM="${VM:-wsl}"   # kept: call sites still pass it positionally

# Which console generation the module declares itself for. 4 here rather than the
# Makefile's 5, because this script drives shadPS4 - a previous-generation emulator that
# refuses a module marked for the current one outright, with
# "e_ident[EI_ABIVERSION] expected 0x00 is (0x2)" and nothing else. See D062.
GENERATION="${GENERATION:-4}"
# Whether the mined census is compiled in, forwarded to the build the same way the
# generation is.
#
# It matters here more than anywhere else because a sweep is a *loop* of builds and runs:
# the corpus adds some thirty thousand targets to every round, and against a loader that
# needs a dozen rounds to get clear of its crashes that is the difference between an
# afternoon and a week. The crashes a sweep is hunting live in the hand-written checks,
# and those are identical either way - so the corpus can be left out of the hunt and put
# back for the run that produces the report.
CORPUS="${CORPUS:-1}"
# Where the module and the exclusion list live, and why this is a parameter.
#
# The list is **per loader**. shadPS4's crashes are not fpPS4's, and a stale entry silently
# hides a check that no longer crashes on the loader being swept - which this script's own
# `--resume` documentation says in as many words.
#
# It was hardcoded to `/tmp/obs`, so sweeping a second loader inherited the first's list and
# a report carried skips belonging to a different emulator. Comparing four loaders is the
# whole point of the compatibility table, and it needs four directories.
BUILD="${BUILD:-/tmp/obs}"
# Whether to keep the exclusion list a previous sweep built, rather than starting empty.
RESUME="${RESUME:-0}"

while [ $# -gt 0 ]; do
    case "$1" in
        --emulator) EMULATOR="$2"; shift 2 ;;
        --out) OUT="$2"; shift 2 ;;
        --generation) GENERATION="$2"; shift 2 ;;
        --build) BUILD="$2"; shift 2 ;;
        --corpus) CORPUS="$2"; shift 2 ;;
        --max-rounds) MAX_ROUNDS="$2"; shift 2 ;;
        --timeout) TIMEOUT="$2"; shift 2 ;;
        --resume) RESUME=1; shift ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

here=$(dirname "$0")

# The build environment lives in scripts/wsl.sh, which replaced multipass. (D199)
. "$(dirname "$0")/wsl.sh"

# Resuming keeps what the last sweep proved.
#
# Each exclusion costs two runs - one to reach the hang and one at double the budget to
# show it is a hang - so a sweep that stops at `--max-rounds` mid-hunt has bought findings
# that are expensive to buy again. Against fpPS4 that was seven exclusions and an hour and
# a half, and starting over would have spent it a second time to reach the same list.
#
# Off by default, and that is deliberate rather than cautious: the first pass has to be made
# with an empty list or a stale exclusion silently hides a check that no longer crashes, and
# the report keeps saying `skip` for something that would now pass. Resuming is a claim that
# the list came from this same loader and build, and only the operator knows that.
vm exec "$VM" -- bash -lc "mkdir -p $BUILD" >/dev/null

if [ "$RESUME" = "1" ]; then
    kept=$(vm exec "$VM" -- bash -lc "wc -l < $BUILD/excludes.txt" | tr -d ' \r')
    echo "resuming with ${kept:-0} exclusions already in the list"
else
    vm exec "$VM" -- bash -lc ": > $BUILD/excludes.txt" >/dev/null
    echo "starting with an empty exclusion list"
fi

round=1
# Whether the current stopping point has already been given a doubled budget, and where it
# stopped when it was. Both belong to the retry rule below.
retried=0
retried_at=
previous_timeout="$TIMEOUT"
while [ "$round" -le "$MAX_ROUNDS" ]; do
    vm exec "$VM" --working-directory "$VM_REPO" -- bash -lc \
        "GEN=$GENERATION CORPUS=$CORPUS BUILD=$BUILD sh scripts/sweep-build.sh > /tmp/sb.log 2>&1" >/dev/null

    # The runner's own output is read, not discarded, because it carries the one fact
    # this loop cannot work out for itself: whether the budget ran out.
    runner_log="${TMPDIR:-/tmp}/obscene-sweep-run.$$"
    VM_MODULE="$BUILD/obscene.module.elf" \
        sh "$here/run-emulator.sh" --emulator "$EMULATOR" --out "$OUT" \
        --timeout "$TIMEOUT" >"$runner_log" 2>&1 || true
    timed_out=$(awk '/^timedout:/ {print $2}' "$runner_log")
    rm -f "$runner_log"

    records=$(wc -l < "$OUT" | tr -d ' ')
    ended=$(grep -c '^OBS|end' "$OUT" || true)

    # A `try` with no matching `res` is the announce-before-attempting invariant paying
    # off: it names the exact call that did not return. Without it a crash says only
    # "somewhere in this run".
    dangling=$(awk -F'|' '
        $2 == "try" { open[$3] = 1 }
        $2 == "res" { delete open[$3] }
        END { for (id in open) { print id; break } }
    ' "$OUT")

    if [ "$ended" -gt 0 ]; then
        echo "round $round: $records records, COMPLETE"
        break
    fi
    # A single run killed by the clock is not evidence of anything.
    #
    # It leaves the same trace as a hang - a `try` with no `res` - so excluding on it would
    # blame whichever check happened to be running when time ran out. On a slow loader with
    # a suite this size that is a healthy check every round, quietly removed and recorded as
    # a crash. This script used to stop here and say "raise --timeout", which is honest and
    # leaves the sweep unable to finish against any loader that genuinely hangs.
    #
    # **Two runs are evidence.** The two are indistinguishable within one round and easy to
    # separate across two: give it more time and see whether it moves. A check that was
    # merely unfinished gets further; a hang stops in exactly the same place however long it
    # is left, because it is not doing anything. So a timeout doubles the budget and retries
    # the same build, and only a stopping point that is *identical under twice the time* is
    # called a hang and excluded.
    #
    # This is the manual step that diagnosed fpPS4 - 300 seconds and 5400 seconds both ended
    # at `007-responsive/libc` on the same record, and eighteen times the budget bought
    # nothing - written down so the loop performs it rather than an operator remembering to.
    # The alternative was a flag asserting "the budget is generous enough to trust", which
    # puts a judgement where a measurement will do, and is wrong exactly when it is most
    # confident.
    if [ "$timed_out" = "1" ]; then
        if [ "$retried" = "1" ] && [ "$dangling" = "$retried_at" ]; then
            echo "round $round: $records records, stopped at \"$dangling\" again"
            echo "  unchanged at ${TIMEOUT}s after ${previous_timeout}s, so it is hung rather"
            echo "  than unfinished. Excluding it."
            retried=0
            TIMEOUT="$previous_timeout"
        elif [ "$retried" = "1" ]; then
            # It moved, so the budget was the problem and the longer one is now the honest
            # setting for the rest of the sweep. Keeping the doubled value rather than
            # restoring it: a loader slow enough to need it once will need it again, and
            # halving it back guarantees another wasted round to rediscover that.
            echo "round $round: $records records, moved on to \"$dangling\" with more time"
            echo "  so the budget was short, not the check. Continuing at ${TIMEOUT}s."
            retried=0
            round=$((round + 1))
            continue
        else
            previous_timeout="$TIMEOUT"
            TIMEOUT=$((TIMEOUT * 2))
            retried=1
            retried_at="$dangling"
            echo "round $round: $records records, the budget ran out at \"$dangling\""
            echo "  retrying at ${TIMEOUT}s to tell a hang from a check that was still going"
            # A retry is a run, so it counts. Otherwise --max-rounds bounds only the
            # rounds that exclude something, and a loader that keeps needing more time
            # keeps doubling it outside any budget the operator set.
            round=$((round + 1))
            continue
        fi
    else
        retried=0
    fi
    if [ -z "$dangling" ]; then
        echo "round $round: $records records, stopped without a dangling try - not a crash inside a call"
        break
    fi

    echo "round $round: $records records, died in $dangling"
    # Appended only if not already there. Two sweeps running at once against the same
    # VM will otherwise each add the same name, and the exclusion list grows copies -
    # harmless to the build, and confusing enough to be worth one grep.
    vm exec "$VM" -- bash -lc \
        "grep -qxF '$dangling' $BUILD/excludes.txt \
            || echo '$dangling' >> $BUILD/excludes.txt" >/dev/null
    round=$((round + 1))
done

echo
echo "excluded:"
vm exec "$VM" -- bash -lc "cat $BUILD/excludes.txt"
echo
echo "report: $OUT"
