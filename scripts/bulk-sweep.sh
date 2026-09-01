#!/bin/sh
# Drives the blind prober to the end of the surface, across as many runs as it takes.
#
# # Why this needs a harness at all
#
# `910-bulk` calls every censused symbol with nothing in its arguments, and some of them
# will not return - a function that dereferences its first argument without checking it
# takes the process with it, and so does one that divides by it. The very first host run
# proved the point at index 159: `div(0, 0)`, SIGFPE, and everything behind it lost.
#
# So one run can only ever reach the first fault. What makes that survivable is the
# announcement: the last `call` record with no answer after it names the function *and*
# carries its index, so the next run starts one past it. Rounds needed is therefore
# "number of functions that fault", not "number of functions".
#
# This is the same shape as `sweep.sh` and its exclusion list, and deliberately so. The
# difference is that an exclusion list names checks and sections, while this carries a
# single integer, because the prober walks one fixed order and a position in it says
# everything.
#
# # The records accumulate; the runs do not
#
# Each round contributes the slice it reached, and they are concatenated. **A single
# round's report is never the answer** - it is a fragment starting wherever the last one
# died. Reading one on its own would silently under-report the surface, which is the
# failure this script exists to prevent someone doing by hand.
#
# # A round costs a fault, so the budget is the constraint
#
# Rounds needed is "number of functions that fault", and they are not spread evenly. This
# section walks the **corpus** - 32,466 symbols - the callable corpus plus the curated census,
# and the faults cluster hard at the front, in libkernel and the pthread family, which is
# where a loader implements most and so has most to get wrong.
#
# Against shadPS4: 31 rounds to cross the first 351 indices, then a single round covering 367
# to 914, then 40 rounds to reach 25,602. So rounds-per-index is not a constant and a budget
# cannot be estimated from an average - set `--max-rounds` generously and use `--resume` when
# it runs out, rather than starting the accumulation again.
#
# # One caution on reading the fault list
#
# "Did not return" is not the same as "faulted". Three different things produce it and the
# announcement cannot tell them apart:
#
#   - **it returns nowhere by design** - `scePthreadExit` at index 81 and `_exit` at 31,733,
#     both of which are behaving correctly and will appear in every list forever;
#   - **it blocked** - a lock or a wait given a null argument, which never comes back;
#   - **it crashed** - the case the prober is actually hunting.
#
# The announcement says only that no answer came back. Which of the three it was needs the
# function's own contract, so the list is input to a judgement rather than the judgement.
#
#   sh scripts/bulk-sweep.sh                       # host build, inside the VM
#   sh scripts/bulk-sweep.sh --target module \
#       --emulator <emulators>/kyty/kyty_emulator.exe
#   sh scripts/bulk-sweep.sh --resume --out reports/bulk-shadps4-run.txt …   # continue one
set -e

TARGET="${TARGET:-host}"
OUT="${OUT:-reports/bulk.txt}"
MAX_ROUNDS="${MAX_ROUNDS:-40}"
VM="${VM:-wsl}"   # kept: call sites still pass it positionally
BUILD="${BUILD:-/tmp/obs}"
# Which console generation the module declares itself for, and 4 is the right default here
# for the same reason it is in `sweep.sh`: a PS4-generation loader reads EI_ABIVERSION before
# a single guest instruction runs, and refuses a GEN=5 module as "not a valid ELF file".
#
# This defaulted to 5 while `sweep.sh` beside it defaulted to 4, and the two drive the same
# loaders. A prober round against shadPS4 was refused at load, produced no records at all, and
# reported "it did not start" - which reads as a fault in the prober, and cost two runs before
# anyone read the loader's own log.
GEN="${GEN:-4}"
EMULATOR="${EMULATOR:-}"
START="${START:-0}"
RESUME="${RESUME:-0}"
# How long one round's run gets. Generous: a full pass makes several hundred calls and some
# of them are slow. The point is that it ends, not that it ends quickly.
RUN_TIMEOUT="${RUN_TIMEOUT:-90}"
# Sections a prober round has no use for.
#
# Every round is a whole run of the program and the accumulated report is the rounds
# concatenated, so anything a round emits is emitted once per round. The census is 35,045
# records, and a 121-round host sweep produced a **354 MB** report of which 4.24 million
# records were 121 identical copies of it, wrapped around 328 answers.
#
# The prober needs none of it - `910-bulk` walks its own table. Excluded through
# `EXTRA_EXCLUDE` rather than the sweep's own list, so a section left out for output volume
# is never left behind in that file as though a crash had been proved there.
BULK_EXCLUDE="${BULK_EXCLUDE:-900-surface}"

while [ $# -gt 0 ]; do
    case "$1" in
        --target) TARGET="$2"; shift 2 ;;
        --out) OUT="$2"; shift 2 ;;
        --emulator) EMULATOR="$2"; shift 2 ;;
        --generation) GEN="$2"; shift 2 ;;
        --start) START="$2"; shift 2 ;;
        --run-timeout) RUN_TIMEOUT="$2"; shift 2 ;;
        --max-rounds) MAX_ROUNDS="$2"; shift 2 ;;
        --resume) RESUME=1; shift ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

here=$(dirname "$0")

# The build environment lives in scripts/wsl.sh, which replaced multipass. (D199)
. "$(dirname "$0")/wsl.sh"

work="${TMPDIR:-/tmp}/obscene-bulk.$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

mkdir -p "$(dirname "$OUT")"
# Resuming continues an accumulation instead of starting one.
#
# `--max-rounds` is the binding constraint here, not the size of the surface. A round ends at
# the first function that does not return, so rounds needed is "number of functions that
# fault" - and against shadPS4 that turned out to be roughly one in five. Seventeen rounds
# reached index 92 of 32,466 - corpus plus curated census, and the section reports that length itself.
#
# Without this, a sweep that runs out of rounds has two bad options: throw the accumulation
# away and pay for it again, or concatenate files by hand - which is exactly the "reading one
# round as though it were the answer" mistake the header of this script exists to prevent
# someone making. So resuming is supported rather than improvised.
#
# `--start` alone is not enough, because the report file is truncated on every invocation.
# Both have to move together, and this reads the index out of the report rather than asking
# the operator to remember it: the last announcement with no answer is the same thing the
# round loop below computes, and a number typed from scrolled-back terminal output is a
# number that can be wrong.
#
# Off by default, like `sweep.sh`'s. Resuming is a claim that the existing report came from
# this same loader and build, and only the operator knows that.
if [ "$RESUME" = "1" ]; then
    [ -f "$OUT" ] || { echo "--resume needs an existing $OUT to continue" >&2; exit 2; }
    # Every announcement with no answer - one per round in an accumulated report, where
    # the round loop below only ever sees the single one that ended its own run.
    pending=$(awk -F'|' '
        $2 == "call" && $6 == "attempt" { pending[$5] = 1 }
        $2 == "call" && $6 != "attempt" { delete pending[$5] }
        END { for (i in pending) print i }
    ' "$OUT")
    if [ -z "$pending" ]; then
        echo "--resume found no unanswered call in $OUT; nothing to continue from" >&2
        exit 2
    fi
    # The highest, taken numerically.
    #
    # The indices are hex, and `sort` on hex text puts `0x9` after `0x10` - which would
    # resume *behind* ground already covered and silently re-walk it. Converted one at a
    # time because `awk` cannot be relied on to read hex (`strtonum` is a gawk extension and
    # this VM's awk is not gawk), and the list is one entry per round, so the loop is short.
    resume_at=0
    for hex in $pending; do
        decimal=$(printf '%d' "$hex")
        if [ "$decimal" -gt "$resume_at" ]; then
            resume_at="$decimal"
        fi
    done
    # One past the last thing that did not come back, which is where the next round would
    # have started had the budget not run out.
    #
    # A run that ended *blocked ahead of the section* contributes no new announcement, so
    # this resumes at the previous round's index and re-walks it. That is correct rather
    # than merely tolerable: the exclusion it added lives in the VM's list and persists, so
    # the retry is the one the round loop would itself have made.
    START=$((resume_at + 1))
    kept=$(grep -c '^OBS|call' "$OUT" 2>/dev/null || echo 0)
    printf 'resuming at index %s, keeping %s records already in %s\n' "$START" "$kept" "$OUT"
else
    : > "$OUT"
fi

next="$START"
round=1
faults=0
blocked=0
finished=0
# Whether the last round produced nothing, and at which index. See the retry below.
barren=0
barren_at=
transient=0

while [ "$round" -le "$MAX_ROUNDS" ]; do
    printf '=== round %d, starting at index %s\n' "$round" "$next"
    slice="$work/round-$round.txt"

    if [ "$TARGET" = host ]; then
        # Built and run in one call. The build's status matters - a compile error that
        # left the previous binary in place would otherwise be reported as a round that
        # found nothing new, which reads as "the sweep converged".
        if ! vm exec "$VM" --working-directory "$VM_REPO" -- bash -lc \
            "make host BULK=1 BULK_START=$next BUILD=$BUILD EXCLUDE='$BULK_EXCLUDE' \
             >/dev/null 2>&1"; then
            echo "the host build failed at index $next" >&2
            exit 1
        fi
        # `|| true` on the *run* only: dying is the expected outcome of a round, and its
        # exit status is not the result. What the round produced is.
        #
        # Bounded, because "did not return" has two causes and only one of them ends the
        # process. A function that blocks on a null argument hangs forever, and an unbounded
        # round waits with it - this sweep sat for fifteen minutes on `fopen` before anyone
        # noticed it was not a slow round. The project already knows this shape: "a probe
        # that hangs loses every check behind it, which has happened twice" (CLAUDE.md).
        #
        # The timeout is inside the VM rather than around the multipass call, so it is the
        # guest process that is bounded. Bounding the outer call would leave the guest
        # running and the next round would be racing it.
        vm exec "$VM" -- bash -lc \
            "timeout $RUN_TIMEOUT $BUILD/obscene-host 2>/dev/null" > "$slice" || true
    else
        [ -n "$EMULATOR" ] || { echo "--target module needs --emulator" >&2; exit 2; }
        # Through sweep-build.sh, not `make module`, and this is not a detail.
        #
        # 910-bulk runs last - it has to, because it is the section that may not return -
        # so **every check that crashes ahead of it blocks the whole sweep**. Building
        # without the exclusion list means the module walks into a known crash in an
        # earlier section and the prober is never reached at all, which presents as
        # "produced no bulk records" and reads as a fault in this script.
        #
        # `build-all.sh` and `make check` have each handed a later step an unexcluded
        # module once already. This was the third place that mistake was available.
        if ! vm exec "$VM" --working-directory "$VM_REPO" -- bash -lc \
            "GEN=$GEN BULK=1 BULK_START=$next BUILD=$BUILD EXTRA_EXCLUDE='$BULK_EXCLUDE' \
             sh scripts/sweep-build.sh >/dev/null 2>&1"; then
            echo "the module build failed at index $next" >&2
            exit 1
        fi
        # `--run-timeout` reaches the emulator too, and did not before.
        #
        # The host path bounded its run and the module path did not, so a loader got
        # run-emulator's own 70-second default. `910-bulk` runs last, after every other
        # section, and on a loader that needs longer than that the module was killed before
        # the prober started - reported as "produced no bulk records", which reads as a fault
        # in the prober rather than a budget that never reached it.
        sh "$here/run-emulator.sh" --emulator "$EMULATOR" --out "$slice"             --timeout "$RUN_TIMEOUT" >/dev/null || true
    fi

    grep '^OBS|' "$slice" >> "$OUT" || true

    # Any records at all means the run happened, so a previous empty round was a
    # transient and not a property of this index.
    if grep -q '^OBS|' "$slice"; then
        barren=0
    fi

    # The last announcement with no answer after it.
    #
    # An attempt record and its result share an index, so a lone attempt is a call that
    # did not return. Matching on the index rather than on the symbol because two
    # libraries may export the same name, and the index is what the next round needs.
    died=$(awk -F'|' '
        $2 == "call" && $6 == "attempt" { pending[$5] = $4 }
        $2 == "call" && $6 != "attempt" { delete pending[$5] }
        END { for (i in pending) print i, pending[i] }
    ' "$slice" | tail -1)

    if [ -n "$died" ]; then
        index=$(printf '%s' "$died" | awk '{print $1}')
        symbol=$(printf '%s' "$died" | awk '{print $2}')
        # The index is reported in hex, and shell arithmetic needs it in decimal.
        decimal=$(printf '%d' "$index")
        printf '    did not return: %s at index %s\n' "$symbol" "$decimal"
        faults=$((faults + 1))
        next=$((decimal + 1))
        round=$((round + 1))
        continue
    fi

    # No lone announcement. Either the section ran out of list, or it never started -
    # and those are opposite outcomes that must not be conflated.
    if grep -q '^OBS|res|910-bulk/probe' "$slice"; then
        printf '    reached the end of the list\n'
        finished=1
        break
    fi

    # Nothing from the section at all, so the run died before reaching it.
    #
    # **This is a different problem with a different answer.** 910-bulk runs last, so any
    # check anywhere above it that does not return stops the module before the prober is
    # ever entered - and no amount of raising the start index helps, because the index only
    # addresses positions inside the section.
    #
    # It is not hypothetical and it is not rare: the first shadPS4 sweep died here on round
    # three, in `015-sync/thread-churn`, a known threshold crash that rounds one and two had
    # happened to survive (BACKLOG 6c). A sweep that gives up at that point reports "no bulk
    # records" for a reason that has nothing to do with the bulk section.
    #
    # So there are two resume mechanisms and each fixes what the other cannot: the exclusion
    # list for crashes *before* the section, the start index for crashes *inside* it. This
    # branch is the first, and it deliberately does not advance `next` - the same index is
    # retried once the offending check is excluded.
    blocker=$(awk -F'|' '
        $2 == "try" { open[$3] = 1 }
        $2 == "res" { delete open[$3] }
        END { for (id in open) { print id; break } }
    ' "$slice")

    if [ -z "$blocker" ]; then
        # A round that produced nothing at all, and this used to end the sweep.
        #
        # There is a third cause, and it is the common one. A run can simply fail: the
        # loader crashes before it opens a window, the module transfer glitches, the
        # emulator loses its device. Nothing was measured and nothing is wrong with the
        # module - the same index run again produces a full slice.
        #
        # That is not a hypothetical. A 31-round sweep ended here at index 351, and
        # re-running that exact index by hand immediately produced 916 records. **One bad run
        # discarded thirty rounds of work**, and reported it in words that name the module
        # rather than the run.
        #
        # So it is retried once, which is the same discipline `sweep.sh` applies to a
        # suspected hang (D144): a single observation of nothing is not evidence of
        # nothing. Only a second empty round at the *same* index concludes.
        if [ "$barren" = "1" ] && [ "$barren_at" = "$next" ]; then
            echo "the run produced nothing twice at index $next; it did not start" >&2
            exit 1
        fi
        barren=1
        barren_at="$next"
        transient=$((transient + 1))
        printf '    produced nothing at index %s; running it again before concluding\n' \
            "$next"
        round=$((round + 1))
        continue
    fi

    printf '    blocked before the section by %s; excluding it and retrying\n' "$blocker"
    vm exec "$VM" -- bash -lc \
        "grep -qxF '$blocker' $BUILD/excludes.txt || echo '$blocker' >> $BUILD/excludes.txt" \
        >/dev/null
    blocked=$((blocked + 1))
    round=$((round + 1))
done

calls=$(grep -c '^OBS|call' "$OUT" 2>/dev/null || echo 0)
answered=$(awk -F'|' '$2 == "call" && $6 != "attempt"' "$OUT" | wc -l | tr -d ' ')
printf '\nrounds: %d, did not return: %d, blocked ahead: %d, empty runs retried: %d, answers: %s\n' \
    "$round" "$faults" "$blocked" "$transient" "$answered"
printf 'records: %s in %s\n' "$calls" "$OUT"

if [ "$finished" -ne 1 ]; then
    printf 'INCOMPLETE: stopped after %d rounds without reaching the end\n' "$MAX_ROUNDS"
    exit 1
fi

# The outcomes, which are the point of the whole exercise.
awk -F'|' '$2 == "call" && $6 != "attempt" { print $6 }' "$OUT" | sort | uniq -c | sort -rn
