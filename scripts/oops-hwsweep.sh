#!/bin/bash
# The iterative sweep, against real hardware.
#
#     bash scripts/oops-hwsweep.sh [max-iterations]
#
# `scripts/sweep.sh` does this for an emulator: run, see which call did not return, add it to
# the exclusion list, run again. A console needs the same loop and a different mechanism - the
# build happens in WSL, the install has to come from Windows because the console connects *in*,
# and the report arrives on the kernel log rather than on a pipe.
#
# The crasher is identified the way this project's first principle says it can be: a `try` with
# no matching `res` is the call that did not return. That is the whole reason announcements are
# unbuffered, and it is what makes an unattended sweep possible at all.
#
# Excluded checks are reported as skips with a reason, so nothing is tidied away silently.
set -u
here="$(cd "$(dirname "$0")" && pwd)"
repo="$(dirname "$here")"
max="${1:-12}"
title="${TITLE:-OBSC00001}"
content="${CONTENT_ID:-IV0002-${title}_00-STOREUPD00000000}"
out="${OUT:-$repo/reports/hardware}"
tool="$repo/tool/target-win/debug/obscene-tool.exe"
# Derived from this script's own location, never written down.
#
# `$repo` is a Git Bash path (`/c/...`); `CARGO_TARGET_DIR` is read by a Windows process and
# `wsl.exe` is handed the Linux form, so all three spellings come from the one root rather than
# from three constants that can disagree - and no absolute path off this machine goes in the
# file. `cygpath` is present wherever this runs, because it runs under Git Bash by definition.
export CARGO_TARGET_DIR="$(cygpath -w "$repo/tool/target-win" 2>/dev/null || printf '%s' "$repo/tool/target-win")"
# a Git Bash drive path to its WSL spelling, the same rewrite `oops-rebuild-pkg.sh` does.
linux_repo="$(printf '%s' "$repo" | sed 's|^/\([a-zA-Z]\)/|/mnt/\1/|')"
staged_pkg="$repo/build-sweep.pkg"
win_pkg="$(cygpath -w "$staged_pkg" 2>/dev/null || printf '%s' "$staged_pkg")"
mkdir -p "$out"

exclude="${EXCLUDE:-}"
best=0
for i in $(seq 1 "$max"); do
    echo "=== iteration $i ==="
    echo "    excluding: ${exclude:-<nothing>}"

    MSYS_NO_PATHCONV=1 wsl.exe -d Ubuntu -- bash -lc \
        "export PATH=\"\$HOME/.cargo/bin:\$PATH\" CARGO_TARGET_DIR=\"\$HOME/obs-tool-target\" CONTENT_ID='$content'; \
         cd '$linux_repo' && ./bin/obscene pkg BUILD=\$HOME/obs GEN=5 HARDWARE=1 EXCLUDE='$exclude' >/tmp/sweep-build.log 2>&1 && \
         cp \$HOME/obs/obscene.pkg '$linux_repo'/build-sweep.pkg" \
        || { echo "    build failed"; tail -5 /tmp/sweep-build.log 2>/dev/null; break; }

    "$tool" hw install "$win_pkg" >/dev/null 2>&1
    log="$out/sweep-$i.log"
    # `hw logs` writes when its window closes, not as it goes, so the wait has to outlast the
    # window. Counting at 95s against a 120s capture read an empty file and stopped a sweep that
    # was in fact making progress - the log had 11,390 records in it moments later.
    capture=100
    ( "$tool" hw logs --seconds "$capture" > "$log" 2>&1 & )
    sleep 3
    "$tool" hw launch "$title" >/dev/null 2>&1
    sleep $((capture + 15))

    # Not `|| echo 0`: grep exits non-zero on zero matches, so that prints the count *and* the
    # fallback, and the two concatenate into something no arithmetic accepts.
    records=$(grep -c "OBS|" "$log" 2>/dev/null)
    records=${records:-0}
    echo "    records: $records"
    [ "$records" -gt "$best" ] && best=$records

    if grep -q "suite complete" "$log" 2>/dev/null; then
        echo "=== COMPLETE on iteration $i ==="
        grep -E "OBS\|tally" "$log" | tail -1
        grep -E "^(OBS\||obscene:)" "$log" > "$out/ps5-full-run.txt"
        exit 0
    fi

    # A `try` with no `res` is the call that did not return.
    last_try=$(grep -E "^OBS\|try\|" "$log" | tail -1 | cut -d'|' -f3)
    if [ -z "$last_try" ]; then
        echo "    no records at all - stopping rather than looping blind"
        break
    fi
    if grep -qE "^OBS\|res\|$(printf '%s' "$last_try" | sed 's/[][\.*^$/]/\&/g')\|" "$log"; then
        echo "    last try returned; the run ended for another reason - stopping"
        break
    fi
    echo "    crasher: $last_try"
    exclude="${exclude:+$exclude }$last_try"
done
echo "=== stopped after $((i)) iterations, best $best records ==="
echo "EXCLUDE=\"$exclude\""
