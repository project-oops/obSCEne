#!/bin/sh
# Runs a built module in an emulator and extracts the report.
#
# The step between `make module` and having something to read.
#
# # Why this is sh and not PowerShell
#
# It was PowerShell, on the reasoning that the emulators are Windows applications. That
# reasoning was wrong: what language drives a process has nothing to do with what platform
# the process is for, and Git Bash launches a Windows executable perfectly well. The rest
# of scripts/ is sh, so the split bought nothing and cost portability.
#
# Converting also deleted a whole class of bug. Windows PowerShell turns any stderr output
# from a native command into a terminating error, so multipass warning about a file it had
# just copied correctly would kill the script - intermittently, which made it read as a new
# fault every time. That workaround needed a shared helper module of its own (D050); in sh
# there is nothing to work around, because stderr is stderr.
#
# # The hazard that replaces it
#
# Git Bash rewrites arguments that look like Unix paths before a Windows program sees them:
# `/tmp/obs/x` becomes `C:/Program Files/Git/tmp/obs/x`. That is why the VM-side paths below
# are passed with MSYS_NO_PATHCONV set. It bites exactly once and then never again, which is
# the sort of thing worth a comment rather than a scar.
# Paths are derived from this script's own location rather than hardcoded, so the
# collection works wherever it is cloned. `$OOPS` is the parent holding all four projects.
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
OOPS="$(cd "$REPO/.." && pwd)"
EMULATORS="${EMULATORS:-$(cd "$OOPS/.." && pwd)/emulators}"

set -e

EMULATOR="${EMULATOR:-$EMULATORS/shadps4/shadPS4.exe}"
OUT="${OUT:-reports/current.txt}"
TIMEOUT="${TIMEOUT:-70}"
# The build environment lives in scripts/wsl.sh, which replaced multipass on 2026-08-26.
# `VM` is kept because call sites still pass it positionally; there is no instance any more.
# (D199)
. "$(dirname "$0")/wsl.sh"

VM="${VM:-wsl}"
# Linux-local, never a /mnt path: a Windows mount cannot carry the execute bit and building
# across it is many times slower. (D012, D198)
# `\`, escaped: this string is expanded by the *inner* WSL shell, not by Git Bash.
# Unescaped it becomes /c/Users/<name>/obs/... - a Windows home that has no module in it, and
# the failure reads as a missing build rather than a mangled path.
# The dollar is escaped because this string is expanded by the *inner* WSL shell, not by
# Git Bash. Unescaped it becomes /c/Users/<name>/obs/... - a Windows home with no module
# in it - and the failure then reads as a missing build rather than a mangled path.
VM_MODULE="${VM_MODULE:-\$HOME/obs/obscene.module.elf}"
MODULE="${MODULE:-}"
# Where to put a screenshot of the emulator window, if one is wanted. Empty means none:
# capturing costs a subprocess and most runs do not need it.
SHOT="${SHOT:-}"
# When to capture, in seconds. Wants to be just after the suite finishes: the summary is
# complete and the detail pages have not started cycling.
SHOT_DELAY="${SHOT_DELAY:-30}"

while [ $# -gt 0 ]; do
    case "$1" in
        --emulator) EMULATOR="$2"; shift 2 ;;
        --module) MODULE="$2"; shift 2 ;;
        --out) OUT="$2"; shift 2 ;;
        --timeout) TIMEOUT="$2"; shift 2 ;;
        --shot) SHOT="$2"; shift 2 ;;
        --shot-delay) SHOT_DELAY="$2"; shift 2 ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

[ -f "$EMULATOR" ] || { echo "no emulator at $EMULATOR - see docs/EMULATORS.md" >&2; exit 1; }

work="${TMPDIR:-/tmp}/obscene-run.$$"
mkdir -p "$work"
trap 'rm -rf "$work"' EXIT

if [ -z "$MODULE" ]; then
    MODULE="$work/obscene.module.elf"
    # The two arguments need opposite treatment, which is the whole trick.
    #
    # The VM-side path must reach multipass as written, so path conversion is off. But
    # that switch is per-command, not per-argument, so the *local* destination would stay
    # a Unix path that Windows multipass cannot resolve - which is what "local target does
    # not exist" meant. Converting it explicitly first gives each argument what it needs.
    local_target="$MODULE"
    command -v cygpath >/dev/null 2>&1 && local_target=$(cygpath -w "$MODULE")
    # `|| true`, and the assertion is the file.
    #
    # WSL writes straight to the Windows filesystem through /mnt, so this is a copy inside
    # one filesystem view rather than a transfer between two. The `|| true` and the existence
    # test below are kept anyway: they were right when multipass reported failure having
    # copied the file correctly, and "test for what you actually want" does not stop being
    # right when the tool improves. (D199)
    vm transfer "${VM}:${VM_MODULE}" "$local_target" || true
    [ -f "$MODULE" ] || {
        echo "could not fetch $VM_MODULE from $VM - has 'make module' been run?" >&2
        exit 1
    }
fi

# How this loader wants to be handed a module.
#
# Not every one takes `-g`. shadPS4 and Kyty do; the SharpEmu/craziiEmu family takes a bare
# positional path and rejects the flag outright. Hardcoding `-g` meant "add an emulator to
# the roster" silently meant "add an emulator that happens to share shadPS4's command line",
# which is not a property anyone checked before assuming it.
#
# Keyed on the executable's name rather than passed by the caller, so `--emulator <path>`
# keeps working for every loader without the caller needing to know which family it is in.
case "$(basename "$EMULATOR" .exe)" in
    SharpEmu|SharpEmu.CLI|CraziiEmu|craziiEmu) launch_flag="" ;;
    # PS5PCEM's runner, ChonkyStation4 and prosperity all take a bare path. Named rather
    # than folded into the default so that adding a loader is a deliberate act with a
    # checked command line - `-g` was the default once and fpPS4 quietly took it for a
    # year's worth of empty results.
    game-run|ChonkyStation4|ps4delta) launch_flag="" ;;
    # `-e` for "decrypted ELF or SELF file name". Passing `-g` here produced a run that
    # started, loaded nothing and reported zero records - which was read for some time as
    # "fpPS4 does not run our module" when it was "fpPS4 was never given our module".
    fpPS4) launch_flag="-e" ;;
    # orbistoun takes a subcommand, not a flag: `orbistoun-cli run <path>`. Unquoted
    # expansion below splits it into three arguments, which is what the other entries here
    # rely on to contribute one or none.
    #
    # `--limit 0` removes *its* execution budget so that this script's timeout is the only
    # clock. Two budgets is one too many: with orbistoun's own 20-second default the suite
    # would be stopped part-way and reported as a guest that ended, which reads like a crash
    # rather than a deadline - the exact confusion `timedout` exists to keep out of the
    # record.
    orbistoun-cli|orbistoun) launch_flag="run --limit 0" ;;
    *) launch_flag="-g" ;;
esac

# Staged under the conventional name whatever it was called on disk.
#
# These loaders look for a package layout around the executable and log what they could not
# find; handing one `obscene.module.elf` is a difference from a real title for no benefit.
# Copying costs nothing and removes a variable that would otherwise be indistinguishable
# from a loader bug.
staged="$work/eboot.bin"
cp "$MODULE" "$staged"

# A module that draws its report never exits on its own, so the timeout firing is the
# expected outcome rather than an error - hence the `|| true`. What matters is the records.
stdout="$work/stdout.txt"
stderr="$work/stderr.txt"

# What Windows calls this process, which is not what the shell will call it. See the note
# below `emulator_pid` for why that distinction is load-bearing.
emulator_name=$(basename "$EMULATOR" .exe)

# Refuse to start on top of one that is already up.
#
# `grep -q` must never read a Windows program's output, and this cost four minutes a run.
#
# `grep -q` exits the moment it matches and closes the pipe. A POSIX writer gets SIGPIPE and
# dies; `tasklist.exe` is a Windows program with no such signal, so it **blocks forever**
# writing the rest of its output to a reader that is gone. The traced symptom is a loop that
# stops mid-iteration and never re-tests its own budget: Kyty ran 4.1 minutes against a
# 100-second timeout, and a 60-second run was still going at five.
#
# Same failure that hung `apt` for thirty-nine minutes today, from the other end of the pipe.
# So: `/FI` to make the output small, and `$(...)` to read it to EOF instead of abandoning it.
process_running() {
    command -v tasklist >/dev/null 2>&1 || return 1
    # MSYS_NO_PATHCONV, or Git Bash rewrites `/FI` into a Windows path and the filter is lost.
    out=$(MSYS_NO_PATHCONV=1 tasklist /FI "IMAGENAME eq $1.exe" /NH 2>/dev/null | tr 'A-Z' 'a-z')
    want=$(printf '%s' "$1.exe" | tr 'A-Z' 'a-z')
    case "$out" in *"$want"*) return 0 ;; esac
    return 1
}

# A leftover instance is not a harmless duplicate: both write to the same report, so the
# extraction below merges two runs into one file and the resume state derived from it is a
# blend of two different attempts. It also looks exactly like a hang from the outside - the
# stale window sits at whatever section it reached and never moves, because nothing owns it.
if process_running "$emulator_name"; then
    echo "a $emulator_name is already running; stopping it before starting a new one" >&2
    taskkill //F //IM "$emulator_name.exe" >/dev/null 2>&1 || true
    sleep 1
fi

# Started in the background so the window can be photographed while it is still up.
#
# obSCEne draws its report, and for a loader with no working text channel that drawing is
# the only report there will be - so the capture has to happen before the timeout kills
# the process, not after.
# Unquoted deliberately: an empty $launch_flag must vanish rather than become an empty
# argument, which these loaders read as a path and refuse.
# shellcheck disable=SC2086
"$EMULATOR" $launch_flag "$staged" >"$stdout" 2>"$stderr" &
emulator_pid=$!

# The shell's process id is not the loader, and for some loaders it never was.
#
# fpPS4 re-execs: eight seconds after launch `kill -0 $!` reports the process gone while
# Windows still has two fpPS4 processes running. Everything downstream then misreads that -
# the wait loop below falls through immediately, `timed_out` comes out 0 on a run that never
# finished, and the `kill` at the end is addressed to a process id that no longer exists. The
# loader is left running, holding its window and writing to the report file, and the next run
# starts on top of it. Two windows, two sets of records, one file, and a run that looks frozen
# because it *is* - abandoned rather than stopped.
#
# `screenshot.sh` already documented this exact trap and fixed it by using the executable's
# basename, which is what Windows calls the process. The lesson was learnt in one place and
# not applied here. (D178)

# Alive if *either* view says so: the shell's, for loaders whose pid is real, and Windows',
# for loaders whose is not. Neither alone is right for every loader in the kit.
emulator_running() {
    if kill -0 "$emulator_pid" 2>/dev/null; then
        return 0
    fi
    if process_running "$emulator_name"; then return 0; fi
    return 1
}

emulator_stop() {
    kill "$emulator_pid" 2>/dev/null || true
    wait "$emulator_pid" 2>/dev/null || true
    if command -v taskkill >/dev/null 2>&1; then
        taskkill //F //IM "$emulator_name.exe" >/dev/null 2>&1 || true
    fi
}

if [ -n "$SHOT" ]; then
    # Timed for the **summary** screen, which is the one worth keeping.
    #
    # obSCEne redraws the summary after every section and leaves it up for the whole run;
    # once the suite finishes it cycles detail pages, three seconds each. So the moment to
    # capture is the instant the run completes: the summary is whole and the cycle has not
    # started.
    #
    # A fixed delay cannot find that moment - thirty seconds caught page one of eight, and
    # any other number is wrong on a different machine. **The report says when it is
    # done.** Waiting for the `end` record is exact, and falls back to the delay if it
    # never arrives, which is the case where a fixed guess was all there ever was.
    waited_for_end=0
    while [ "$waited_for_end" -lt "$SHOT_DELAY" ]; do
        # Unanchored, like the extraction below: records arrive embedded in a loader's own
        # log lines, not at the start of one. `^OBS|end` matched nothing, so this loop
        # and the run loop below both waited out their full budget on every successful
        # run - the screenshot landed late and a sweep round cost its whole timeout.
        if grep -q "OBS|end" "$stdout" 2>/dev/null; then
            break
        fi
        sleep 1
        waited_for_end=$((waited_for_end + 1))
    done
    # By name, not by the shell's process id.
    #
    # `$!` under Git Bash is a *bash* process id, and the Windows tooling that owns window
    # handles knows nothing about it - so passing it looked correct and found no window
    # every time. The executable's basename is what Windows calls the process.
    sh "$(dirname "$0")/screenshot.sh"         --process "$(basename "$EMULATOR" .exe)" --out "$SHOT" --delay 1 || true
fi

# Wait for the report to end, the loader to stop, or the budget to run out.
#
# **The `end` record is the signal, and waiting past it is pure waste.** A module that draws
# its report never exits on its own - it cycles result pages for as long as it is left alive -
# so the process staying up says nothing about whether the suite finished. For a long time the
# only stopping condition was the budget, which meant every successful run cost its whole
# timeout: shadPS4 finishes the suite in about a minute and was being given two hundred
# seconds, and the sweep multiplies that by a round.
#
# The screenshot path above has waited on this record for a while, for the same reason and
# with the same comment. It simply was not applied to the run itself.
# The budget is wall-clock, and it has to be.
#
# This loop used to count its own iterations and call the total seconds, which was true only
# while the body was one `grep` on a local file. It stopped being true the moment a liveness
# probe went in beside it: `tasklist` costs about two seconds warm and eighty-five cold, so a
# stated sixty-second budget quietly became four minutes and more. The run on screen outlived
# every number this script reported about it.
#
# Reading the clock costs nothing and cannot drift, so whatever the body grows into later,
# `TIMEOUT` keeps meaning seconds. (D178)
started=$(date +%s)
waited=0
finished_early=0

# Liveness is cheap for loaders whose process id is real and expensive for the rest, so it is
# probed on a slow cadence and remembered in between. `emulator_running` tries `kill -0`
# first, which costs nothing and answers for shadPS4 and PS5PCEM; only when that fails does it
# reach for `tasklist`, and only every OBS_PROBE_SECONDS seconds. Six calls a run, not sixty.
OBS_PROBE_SECONDS=10
emulator_alive=1
probe_at=0

while [ "$waited" -lt "$TIMEOUT" ]; do
    if grep -q "OBS|end" "$stdout" 2>/dev/null || grep -q "OBS|end" "$stderr" 2>/dev/null; then
        finished_early=1
        break
    fi
    if [ "$waited" -ge "$probe_at" ]; then
        if emulator_running; then
            emulator_alive=1
        else
            emulator_alive=0
        fi
        probe_at=$((waited + OBS_PROBE_SECONDS))
    fi
    if [ "$emulator_alive" != "1" ]; then
        break
    fi
    sleep 1
    waited=$(( $(date +%s) - started ))
done
# Whether the budget ran out, or the loader stopped on its own.
#
# These are opposite facts and the report cannot tell them apart: a run killed part-way
# through a check leaves exactly the same trace as a check that hung - a `try` with no
# `res`. `sweep.sh` excludes on that trace, so with a budget too small for the suite it
# excludes a healthy check every round and calls it a crash.
#
# So the runner says which happened, because only the runner knows.
#
# A run that ended on the `end` record did **not** time out however long it took to get
# there - that is the whole point of watching for it, and reporting otherwise would make the
# sweep exclude a check that finished.
if [ "$finished_early" = "1" ]; then
    timed_out=0
elif [ "$waited" -ge "$TIMEOUT" ]; then
    timed_out=1
else
    timed_out=0
fi
emulator_stop

# Records are interleaved with the emulator's own logging, so each line is cut at the
# prefix rather than matched whole. A value field can occasionally carry log text as a
# result - see docs/examples - so one captured value is worth less than the record round it.
mkdir -p "$(dirname "$OUT")"
# Both streams, because which one a loader uses is its business and not ours.
#
# orbistoun puts guest output on *stderr* by design: it speaks a newline-delimited protocol
# over its own stdout, and guest bytes interleaved into that would corrupt it permanently.
# Reading only stdout there would produce a zero-record run that looks exactly like "the
# module never reported" - the most misleading failure this script could have, since the
# report would be sitting in a file two lines away.
# Nulls stripped before anything looks for records, and this was not cosmetic.
#
# fpPS4 writes NUL bytes into its log. `grep` sees one, decides the stream is binary, prints
# "Binary file (standard input) matches" and emits **nothing** - so a run that produced a
# perfectly good report yielded an empty one, and every fpPS4 result this project has
# recorded was thinned by it. A sweep over that saw no records, concluded there was no
# dangling call, and stopped after one round having found nothing.
#
# The failure is silent in the worst way: the emulator ran, the records existed, and the
# extraction threw them away without an error anybody would notice.
cat "$stdout" "$stderr" 2>/dev/null | tr -d '\000' | grep -o 'OBS|.*' > "$OUT" || true

records=$(wc -l < "$OUT" | tr -d ' ')
ended=$(grep -c '^OBS|end' "$OUT" || true)
crashes=$(cat "$stdout" "$stderr" 2>/dev/null | grep -c 'Unhandled Exception' || true)

# Did the loader take the module at all?
#
# Zero records has two causes that need opposite responses, and nothing here distinguished
# them. A run that started and died early is a result - the announcement names what killed
# it. A run the loader **refused** is not a result at all: no guest instruction executed, so
# there is nothing to read and no index to resume past.
#
# The second is easy to cause and was reported as the first. shadPS4 reads EI_ABIVERSION
# before it does anything else and rejects a current-generation module outright; the sweep
# above it saw an empty report, found no dangling call, and printed "it did not start" -
# which reads as a fault in the prober. Two runs were spent there.
#
# Matched on the loaders' own refusal wording rather than on an exit status, because a loader
# that refuses a module still exits cleanly and still draws a window.
refused=$(cat "$stdout" "$stderr" 2>/dev/null | grep -c -e 'is not valid ELF file' \
    -e 'Failed to load game' -e 'not a valid PS4 executable' -e 'Unsupported ABI version' \
    || true)
if [ "$records" -eq 0 ] && [ "$refused" -gt 0 ]; then
    echo "the loader refused the module - it was never run. Its own words:" >&2
    cat "$stdout" "$stderr" 2>/dev/null | grep -e 'EI_ABIVERSION' \
        -e 'is not valid ELF file' -e 'Failed to load game' | tail -3 >&2
    echo "(a current-generation module is refused by a previous-generation loader: try GEN=4)" >&2
fi

echo "finished: $([ "$finished_early" = "1" ] && echo "on the end record after ${waited}s" || echo "no end record")"
echo "records: $records"
echo "ended:   $ended"
echo "crashes: $crashes"
echo "timedout: $timed_out"
echo "report:  $OUT"
