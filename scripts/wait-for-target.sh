#!/bin/bash
# Wait for the jailbreak payloads to come up. Checks every 2 minutes for 3 hours.
#
# Distinguishes three states rather than two, because a poller that cannot tell "not yet" from
# "my own tooling is broken" loops silently and reports nothing - which is what happened when
# the console registration vanished and every check returned "no consoles registered".
# Paths are derived from this script's own location rather than hardcoded, so the
# collection works wherever it is cloned. `$OOPS` is the parent holding all four projects.
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
OOPS="$(cd "$REPO/.." && pwd)"

cd "$OOPS/obscene/tool"
export CARGO_TARGET_DIR="$OOPS/obscene/tool/target-win"
TOOL=./target-win/debug/obscene-tool.exe
for attempt in $(seq 1 90); do
    out="$("$TOOL" hw check 2>&1)"
    if printf '%s' "$out" | grep -q "no consoles registered"; then
        "$TOOL" hw register 192.168.1.211 --name ps5 >/dev/null 2>&1
        echo "attempt $attempt: registration had gone; re-registered"
    elif printf '%s' "$out" | grep -q "up   shsrv"; then
        echo "JAILBREAK IS UP (attempt $attempt)"
        printf '%s\n' "$out" | tail -7
        exit 0
    fi
    sleep 120
done
echo "still down after 3 hours"
exit 1
