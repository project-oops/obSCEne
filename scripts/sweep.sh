#!/bin/bash
# scripts/sweep.sh - the three-way hardware sweep behind `./bin/obscene sweep`.
#
# Runs the three delivery legs against the console in series - payload, package, native
# eboot - one after the other. Each leg writes two files under reports/hardware/, sharing
# one sweep timestamp:
#
#   <TS>-<leg>.log       the entire run: build, send/install, and the device system log
#   <TS>-<leg>.obs.log   the OBS| report records from that run, and nothing else
#
# Six files for a full sweep. <leg> is payload | pkg | eboot; <TS> is YYYYmmdd-HHMMSS.
#
# Which tool binds which interface is the one subtlety (repository CLAUDE.md): the package
# install has the console fetch *from us*, which needs the Windows-native tool on the LAN
# address; everything else connects *out* and runs from the Linux tool in WSL.
#
#   --seconds N   per-leg run/capture window, stops early on the end record  (default 240)
#   --corpus 0|1  mined-census on/off - 0 is the fast behavioural pass        (default 0)
#   --only LEGS   a subset, space-separated, e.g. --only "pkg eboot"          (default all)
set -u

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"

# Re-enter WSL if not already there - Git Bash has no clang. A script file rather than
# `bash -lc`, which mangles args under Git Bash (repository CLAUDE.md).
if ! grep -qi microsoft /proc/version 2>/dev/null; then
    [ -n "${OBS_SWEEP_REENTERED:-}" ] && { echo "sweep.sh: re-entered WSL and still not in WSL" >&2; exit 1; }
    linux="$(printf '%s' "$HERE/sweep.sh" | sed 's|^/\([a-zA-Z]\)/|/mnt/\1/|')"
    # Pick the build distro: an explicit WSL_DISTRO wins; otherwise prefer `oops-builder` (the
    # one this repo builds in) and fall back to Ubuntu. `wsl.exe -l` prints UTF-16 with CRs, so
    # strip the null bytes and carriage returns before matching.
    dist="${WSL_DISTRO:-}"
    if [ -z "$dist" ]; then
        if wsl.exe -l -q 2>/dev/null | tr -d '\000\r' | grep -qix 'oops-builder'; then
            dist="oops-builder"
        else
            dist="Ubuntu"
        fi
    fi
    echo "sweep.sh: not in WSL - re-entering $dist"
    exec wsl.exe -d "$dist" -- env OBS_SWEEP_REENTERED=1 bash "$linux" "$@"
fi

SECONDS_WIN=320
CORPUS_VAL="${CORPUS:-0}"
legs="payload pkg eboot"
while [ $# -gt 0 ]; do
    case "$1" in
        --seconds) SECONDS_WIN="$2"; shift 2 ;;
        --corpus)  CORPUS_VAL="$2"; shift 2 ;;
        --only)    legs="$2"; shift 2 ;;
        -h|--help) sed -n '2,18p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "sweep.sh: unknown option $1 (try --help)" >&2; exit 2 ;;
    esac
done

[ -f "$HOME/.cargo/env" ] && . "$HOME/.cargo/env"
export PATH="$HOME/.cargo/bin:$PATH"
export CORPUS="$CORPUS_VAL"
B="${BUILD:-$HOME/obs-sweep}"
TT="$HOME/obs-tool-target"
export CARGO_TARGET_DIR="$TT"

outdir="$REPO/reports/hardware"
mkdir -p "$outdir" "$REPO/build"
TS="$(date +%Y%m%d-%H%M%S)"

# Linux tool for everything that connects out; Windows tool for the package install (inbound).
LTOOL="$TT/release/obscene-tool"
WEXE="$REPO/tool/target-win/release/obscene-tool.exe"

ensure_tools() {
    [ -x "$LTOOL" ] || { echo "building linux tool..."; ( cd "$REPO/tool" && cargo build --release --quiet ) || true; }
    if [ ! -x "$WEXE" ]; then
        echo "building windows tool..."
        ( cd "$REPO/tool" && CARGO_TARGET_DIR="$REPO/tool/target-win" WSLENV=CARGO_TARGET_DIR/p \
            cargo.exe build --release --bin obscene-tool >/dev/null 2>&1 ) || true
    fi
}

# Poll the growing device-log temp file; stop the reader and runner once the report ends or a
# fatal signal lands, or when the window elapses.
#
# `reader` and `runner` must be the tool processes' own PIDs, not a subshell wrapping a
# pipeline: a run that ends fast (a guarded eboot now stops on OBS|end in under a minute)
# reaches the kill mid-stream, and kill -9 on a `( cmd | tr )` subshell leaves the tool and
# the `tr` orphaned - the orphaned `tr` holds the tee pipe open so the whole sweep hangs
# before it writes the .obs.log, and an orphaned `hw logs` keeps klogsrv's single reader slot
# so the next leg captures nothing. Killing the tool PID directly reaps it. So each leg writes
# reader/runner output to files and captures `$!` of the bare tool, with no pipe on either.
poll_and_stop() {
    local tmp="$1" reader="$2" runner="$3" i=0
    while [ "$i" -lt "$SECONDS_WIN" ]; do
        sleep 2; i=$((i + 2))
        grep -qaE '^OBS\|(end|tally)\||exited on signal|# fault address:' "$tmp" 2>/dev/null && { sleep 2; break; }
        kill -0 "$reader" 2>/dev/null || break
    done
    kill -9 "$runner" 2>/dev/null || true
    kill -9 "$reader" 2>/dev/null || true
    wait "$reader" 2>/dev/null || true
    wait "$runner" 2>/dev/null || true
}

leg_payload() {
    echo "=== BUILD: make payload (HARDWARE=1 CORPUS=$CORPUS) ==="
    rm -f "$B/obscene-payload.elf" "$B/obscene.elf"
    make -C "$REPO" payload HARDWARE=1 BUILD="$B" TOOL_TARGET="$TT" 2>&1
    local elf="$B/obscene-payload.elf"; [ -f "$elf" ] || elf="$B/obscene.elf"
    [ -f "$elf" ] || { echo "sweep: no payload elf built"; return 1; }
    echo "payload elf: $elf ($(stat -c %s "$elf") bytes)"
    echo "=== SEND (elfldr) + DEVICE LOG (up to ${SECONDS_WIN}s) ==="
    local tmp trun; tmp="$(mktemp)"; trun="$(mktemp)"
    "$LTOOL" hw logs --seconds "$((SECONDS_WIN + 15))" >"$tmp" 2>/dev/null &
    local reader=$!; sleep 3
    "$LTOOL" hw send "$elf" --seconds "$SECONDS_WIN" >"$trun" 2>&1 &
    local runner=$!
    poll_and_stop "$tmp" "$reader" "$runner"
    tr -d '\r' <"$trun"; rm -f "$trun"
    echo "=== DEVICE SYSTEM LOG (payload) ==="; tr -d '\r' <"$tmp"; rm -f "$tmp"
}

leg_pkg() {
    echo "=== BUILD: make pkg (ps4 package, HARDWARE=1 CORPUS=$CORPUS) ==="
    make -C "$REPO" pkg HARDWARE=1 BUILD="$B" TOOL_TARGET="$TT" 2>&1
    [ -f "$B/obscene.pkg" ] || { echo "sweep: no pkg built"; return 1; }
    cp -f "$B/obscene.pkg" "$REPO/build/obscene.pkg"
    local win_pkg; win_pkg="$(wslpath -w "$REPO/build/obscene.pkg")"
    echo "pkg: $REPO/build/obscene.pkg"
    echo "=== close OBSC00001 + INSTALL (Windows serve, console fetches) ==="
    ( cd /mnt/c && "$WEXE" hw close-app OBSC00001 2>&1 | tr -d '\r' ) || true
    ( cd /mnt/c && "$WEXE" hw install "$win_pkg" --seconds 80 2>&1 | tr -d '\r' )
    echo "=== LAUNCH OBSC00001 + DEVICE LOG (up to ${SECONDS_WIN}s) ==="
    local tmp trun; tmp="$(mktemp)"; trun="$(mktemp)"
    "$LTOOL" hw logs --seconds "$((SECONDS_WIN + 15))" >"$tmp" 2>/dev/null &
    local reader=$!; sleep 3
    "$LTOOL" hw launch OBSC00001 --seconds "$SECONDS_WIN" >"$trun" 2>&1 &
    local runner=$!
    poll_and_stop "$tmp" "$reader" "$runner"
    tr -d '\r' <"$trun"; rm -f "$trun"
    echo "=== DEVICE SYSTEM LOG (pkg) ==="; tr -d '\r' <"$tmp"; rm -f "$tmp"
}

leg_eboot() {
    echo "=== BUILD: make native (gen-5 eboot title, CORPUS=$CORPUS) ==="
    make -C "$REPO" native BUILD="$B" TOOL_TARGET="$TT" 2>&1
    local dir="$B/native/PPSA99980"
    [ -d "$dir" ] || { echo "sweep: no native title dir at $dir"; return 1; }
    echo "native dir: $dir"
    echo "=== close PPSA99980 + UPLOAD (install-native, FTP out) ==="
    ( "$LTOOL" hw close-app PPSA99980 2>&1 | tr -d '\r' ) || true
    ( "$LTOOL" hw install-native "$dir" 2>&1 | tr -d '\r' )
    echo "waiting 20s for ShadowMountPlus to register the title..."; sleep 20
    echo "=== LAUNCH PPSA99980 + DEVICE LOG (up to ${SECONDS_WIN}s) ==="
    local tmp trun; tmp="$(mktemp)"; trun="$(mktemp)"
    "$LTOOL" hw logs --seconds "$((SECONDS_WIN + 15))" >"$tmp" 2>/dev/null &
    local reader=$!; sleep 3
    "$LTOOL" hw launch PPSA99980 --seconds "$SECONDS_WIN" >"$trun" 2>&1 &
    local runner=$!
    poll_and_stop "$tmp" "$reader" "$runner"
    tr -d '\r' <"$trun"; rm -f "$trun"
    echo "=== DEVICE SYSTEM LOG (eboot) ==="; tr -d '\r' <"$tmp"; rm -f "$tmp"
}

# Run one leg: tee the whole thing to the full log, then extract the OBS records (dedup with
# awk, preserving order, so a payload that reports on both the socket and the log is not
# doubled).
run_leg() {
    local leg="$1" fn="$2"
    local full="$outdir/${TS}-${leg}.log"
    local obs="$outdir/${TS}-${leg}.obs.log"
    echo
    echo "################  sweep leg: $leg  ->  ${TS}-${leg}.log  ################"
    {
        echo "### obSCEne sweep :: leg=$leg :: corpus=$CORPUS :: $(date -u +%FT%TZ)"
        "$fn"
    } 2>&1 | tee "$full"
    grep -aE '^OBS\|' "$full" 2>/dev/null | awk '!seen[$0]++' > "$obs" || true
    local n end crash; n=$(grep -acE '^OBS\|' "$obs" 2>/dev/null || true)
    end=$(grep -acE '^OBS\|end' "$obs" 2>/dev/null || true)
    crash=$(grep -acE '\|crash\|' "$obs" 2>/dev/null || true)
    echo ">>> $leg: ${n:-0} OBS records, end=${end:-0}, crashes(caught)=${crash:-0}  ->  ${TS}-${leg}.obs.log"
}

ensure_tools
echo "obSCEne sweep $TS  (corpus=$CORPUS, window=${SECONDS_WIN}s, legs: $legs)"
for leg in $legs; do
    case "$leg" in
        payload) run_leg payload leg_payload ;;
        pkg)     run_leg pkg     leg_pkg ;;
        eboot)   run_leg eboot   leg_eboot ;;
        *) echo "sweep: unknown leg '$leg' (want payload|pkg|eboot)" >&2 ;;
    esac
done

echo
echo "=== sweep $TS complete - files under reports/hardware/ ==="
ls -la "$outdir"/${TS}-*.log "$outdir"/${TS}-*.obs.log 2>/dev/null
