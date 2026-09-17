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
#   --seconds N      per-leg run/capture window, stops early on the end record  (default 240)
#   --corpus 0|1     mined-census on/off - 0 is the fast behavioural pass        (default 0)
#   --only LEGS      a subset, space-separated, e.g. --only "pkg eboot"          (default all)
#   --deploy-only    skip rebuild and deploy pre-built artifacts                 (default build)
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
    export MSYS_NO_PATHCONV=1
    echo "sweep.sh: not in WSL - re-entering $dist"
    exec wsl.exe -d "$dist" -- env OBS_SWEEP_REENTERED=1 bash "$linux" "$@"
fi

SECONDS_WIN=320
CORPUS_VAL="${CORPUS:-0}"
legs="payload pkg eboot"
do_build=1
while [ $# -gt 0 ]; do
    case "$1" in
        --seconds) SECONDS_WIN="$2"; shift 2 ;;
        --corpus)  CORPUS_VAL="$2"; shift 2 ;;
        --only)    legs="$2"; shift 2 ;;
        --deploy-only) do_build=0; shift ;;
        -h|--help) sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
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

# Title identity sourced from app.env, per standard OOPS convention (REQ-20260911T0940Z-e39a).
app_env="$REPO/app.env"
[ -f "$app_env" ] && . "$app_env"
TITLE_CODE="${TITLE_CODE:-O00001}"
PKG_TITLE_ID="${PKG_TITLE_ID:-ORB${TITLE_CODE}}"
NATIVE_TITLE_ID="${NATIVE_TITLE_ID:-${TITLE_ID:-PRO${TITLE_CODE}}}"

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
    local tmp="$1" reader="$2" runner="$3" init_bytes="${4:-0}" i=0
    while [ "$i" -lt "$SECONDS_WIN" ]; do
        sleep 2; i=$((i + 2))
        tail -c +$((init_bytes + 1)) "$tmp" 2>/dev/null | grep -qaE '^OBS\|end\||exited on signal|# fault address:' && { sleep 2; break; }
        kill -0 "$reader" 2>/dev/null || break
    done
    kill -9 "$runner" 2>/dev/null || true
    kill -9 "$reader" 2>/dev/null || true
    wait "$reader" 2>/dev/null || true
    wait "$runner" 2>/dev/null || true
}

leg_payload() {
    local elf=""
    if [ "$do_build" = 1 ]; then
        echo "=== BUILD: make payload (HARDWARE=1 CORPUS=$CORPUS) ==="
        rm -f "$B/obscene-probe-prospero.elf"
        make -C "$REPO" payload HARDWARE=1 BUILD="$B" TOOL_TARGET="$TT" BUILD_ID="swp$TS" 2>&1
        elf="$B/obscene-probe-prospero.elf"
    else
        echo "=== skipping build (--deploy-only) ==="
        if [ -n "${BUILD:-}" ] && [ -f "$B/obscene-probe-prospero.elf" ]; then
            elf="$B/obscene-probe-prospero.elf"
        elif [ -f "$REPO/build/obscene-probe-prospero.elf" ]; then
            elf="$REPO/build/obscene-probe-prospero.elf"
        fi
    fi
    [ -n "$elf" ] && [ -f "$elf" ] || { echo "sweep: no payload elf found"; return 1; }
    echo "payload elf: $elf ($(stat -c %s "$elf") bytes)"
    ( "$LTOOL" hw close-app "$PKG_TITLE_ID" 2>&1 | tr -d '\r' ) || true
    ( "$LTOOL" hw close-app "$NATIVE_TITLE_ID" 2>&1 | tr -d '\r' ) || true
    ( "$LTOOL" hw close-app OBSC00001 2>&1 | tr -d '\r' ) || true
    ( "$LTOOL" hw close-app PPSA99980 2>&1 | tr -d '\r' ) || true
    ( "$LTOOL" hw close-app GLHW00001 2>&1 | tr -d '\r' ) || true
    ( "$LTOOL" hw close-app GLCB00001 2>&1 | tr -d '\r' ) || true
    echo "=== SEND (elfldr) + DEVICE LOG (up to ${SECONDS_WIN}s) ==="
    local tmp trun; tmp="$(mktemp)"; trun="$(mktemp)"
    "$LTOOL" hw logs --seconds "$((SECONDS_WIN + 15))" >"$tmp" 2>/dev/null &
    local reader=$!; sleep 3
    local init_bytes=0
    [ -f "$tmp" ] && init_bytes=$(stat -c %s "$tmp" 2>/dev/null || echo 0)
    "$LTOOL" hw send "$elf" --seconds "$SECONDS_WIN" >"$trun" 2>&1 &
    local runner=$!
    (
        for i in $(seq 1 40); do
            sleep 1
            python3 -c '
import socket
try:
    s = socket.create_connection(("192.168.1.211", 9899), timeout=2)
    s.sendall(b"HELLO_SWEEP_PORT_9899\n")
    data = s.recv(1024)
    s.close()
    print("Port 9899 echo ok:", data)
    exit(0)
except Exception:
    exit(1)
' 2>/dev/null && break
        done
    ) &
    local probe_pid=$!
    poll_and_stop "$tmp" "$reader" "$runner" "$init_bytes"
    kill "$probe_pid" 2>/dev/null || true
    tr -d '\r' <"$trun"; rm -f "$trun"
    echo "=== DEVICE SYSTEM LOG (payload) ==="
    tail -c +$((init_bytes + 1)) "$tmp" 2>/dev/null | tr -d '\r'; rm -f "$tmp"
}

leg_pkg() {
    if [ "$do_build" = 1 ]; then
        echo "=== BUILD: make pkg (ps4 package, HARDWARE=1 CORPUS=$CORPUS) ==="
        make -C "$REPO" pkg HARDWARE=1 BUILD="$B" TOOL_TARGET="$TT" 2>&1
        [ -f "$B/obscene-probe-orbis.pkg" ] || { echo "sweep: no pkg built"; return 1; }
        cp -f "$B/obscene-probe-orbis.pkg" "$REPO/build/obscene-probe-orbis.pkg"
    else
        echo "=== skipping build (--deploy-only) ==="
        if [ -n "${BUILD:-}" ] && [ -f "$B/obscene-probe-orbis.pkg" ]; then
            cp -f "$B/obscene-probe-orbis.pkg" "$REPO/build/obscene-probe-orbis.pkg"
        fi
    fi
    [ -f "$REPO/build/obscene-probe-orbis.pkg" ] || { echo "sweep: no pkg found at $REPO/build/obscene-probe-orbis.pkg"; return 1; }
    local win_pkg; win_pkg="$(wslpath -w "$REPO/build/obscene-probe-orbis.pkg")"
    echo "pkg: $REPO/build/obscene-probe-orbis.pkg ($(stat -c %s "$REPO/build/obscene-probe-orbis.pkg") bytes)"
    echo "=== close $PKG_TITLE_ID & $NATIVE_TITLE_ID + INSTALL (Windows serve, console fetches) ==="
    ( "$LTOOL" hw close-app "$NATIVE_TITLE_ID" 2>&1 | tr -d '\r' ) || true
    ( "$LTOOL" hw close-app PPSA99980 2>&1 | tr -d '\r' ) || true
    ( "$LTOOL" hw close-app GLHW00001 2>&1 | tr -d '\r' ) || true
    ( "$LTOOL" hw close-app GLCB00001 2>&1 | tr -d '\r' ) || true
    ( cd /mnt/c && "$WEXE" hw close-app "$PKG_TITLE_ID" 2>&1 | tr -d '\r' ) || true
    ( cd /mnt/c && "$WEXE" hw close-app OBSC00001 2>&1 | tr -d '\r' ) || true
    ( cd /mnt/c && "$WEXE" hw install "$win_pkg" --seconds 80 2>&1 | tr -d '\r' )
    echo "=== LAUNCH $PKG_TITLE_ID + DEVICE LOG (up to ${SECONDS_WIN}s) ==="
    local tmp trun; tmp="$(mktemp)"; trun="$(mktemp)"
    "$LTOOL" hw logs --seconds "$((SECONDS_WIN + 15))" >"$tmp" 2>/dev/null &
    local reader=$!; sleep 3
    local init_bytes=0
    [ -f "$tmp" ] && init_bytes=$(stat -c %s "$tmp" 2>/dev/null || echo 0)
    "$LTOOL" hw launch "$PKG_TITLE_ID" --seconds "$SECONDS_WIN" >"$trun" 2>&1 &
    local runner=$!
    poll_and_stop "$tmp" "$reader" "$runner" "$init_bytes"
    tr -d '\r' <"$trun"; rm -f "$trun"
    echo "=== DEVICE SYSTEM LOG (pkg) ==="
    tail -c +$((init_bytes + 1)) "$tmp" 2>/dev/null | tr -d '\r'; rm -f "$tmp"
}

leg_eboot() {
    local dir=""
    if [ "$do_build" = 1 ]; then
        echo "=== BUILD: make native (gen-5 eboot title, CORPUS=$CORPUS) ==="
        make -C "$REPO" native BUILD="$B" TOOL_TARGET="$TT" 2>&1
        dir="$B/prospero/$NATIVE_TITLE_ID"
    else
        echo "=== skipping build (--deploy-only) ==="
        if [ -n "${BUILD:-}" ] && [ -d "$B/prospero/$NATIVE_TITLE_ID" ]; then
            dir="$B/prospero/$NATIVE_TITLE_ID"
        elif [ -d "$REPO/build/prospero/$NATIVE_TITLE_ID" ]; then
            dir="$REPO/build/prospero/$NATIVE_TITLE_ID"
        elif [ -n "${BUILD:-}" ] && [ -d "$B/prospero/PPSA99980" ]; then
            dir="$B/prospero/PPSA99980"
        elif [ -d "$REPO/build/prospero/PPSA99980" ]; then
            dir="$REPO/build/prospero/PPSA99980"
        fi
    fi
    [ -n "$dir" ] && [ -d "$dir" ] || { echo "sweep: no native title dir found"; return 1; }
    echo "native dir: $dir"
    echo "=== close $PKG_TITLE_ID & $NATIVE_TITLE_ID + UPLOAD (install-native, FTP out) ==="
    ( "$LTOOL" hw close-app "$PKG_TITLE_ID" 2>&1 | tr -d '\r' ) || true
    ( "$LTOOL" hw close-app OBSC00001 2>&1 | tr -d '\r' ) || true
    ( cd /mnt/c && "$WEXE" hw close-app "$PKG_TITLE_ID" 2>&1 | tr -d '\r' ) || true
    ( cd /mnt/c && "$WEXE" hw close-app OBSC00001 2>&1 | tr -d '\r' ) || true
    ( "$LTOOL" hw close-app "$NATIVE_TITLE_ID" 2>&1 | tr -d '\r' ) || true
    ( "$LTOOL" hw close-app PPSA99980 2>&1 | tr -d '\r' ) || true
    ( "$LTOOL" hw close-app GLHW00001 2>&1 | tr -d '\r' ) || true
    ( "$LTOOL" hw close-app GLCB00001 2>&1 | tr -d '\r' ) || true
    ( "$LTOOL" hw install-native "$dir" 2>&1 | tr -d '\r' )
    echo "waiting 20s for ShadowMountPlus to register the title..."; sleep 20
    echo "=== LAUNCH $NATIVE_TITLE_ID + DEVICE LOG (up to ${SECONDS_WIN}s) ==="
    local tmp trun; tmp="$(mktemp)"; trun="$(mktemp)"
    "$LTOOL" hw logs --seconds "$((SECONDS_WIN + 15))" >"$tmp" 2>/dev/null &
    local reader=$!; sleep 3
    local init_bytes=0
    [ -f "$tmp" ] && init_bytes=$(stat -c %s "$tmp" 2>/dev/null || echo 0)
    "$LTOOL" hw launch "$NATIVE_TITLE_ID" --seconds "$SECONDS_WIN" >"$trun" 2>&1 &
    local runner=$!
    poll_and_stop "$tmp" "$reader" "$runner" "$init_bytes"
    tr -d '\r' <"$trun"; rm -f "$trun"
    echo "=== DEVICE SYSTEM LOG (eboot) ==="
    tail -c +$((init_bytes + 1)) "$tmp" 2>/dev/null | tr -d '\r'; rm -f "$tmp"
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
    sed -n -e 's/.*\(OBS|.*\)/\1/p' "$full" 2>/dev/null | awk '!seen[$0]++' > "$obs" || true
    local n end crash; n=$(grep -acE '^OBS\|' "$obs" 2>/dev/null || true)
    end=$(grep -acE '^OBS\|end' "$obs" 2>/dev/null || true)
    crash=$(grep -acE '\|crash\|' "$obs" 2>/dev/null || true)
    if [ "$end" -eq 0 ] && [ "$leg" = "payload" ]; then
        echo "sweep: payload log incomplete, pulling /mnt/usb0/obscene/report.txt..."
        local usbtmp; usbtmp="$(mktemp)"
        if "$LTOOL" hw pull /mnt/usb0/obscene/report.txt --into "$usbtmp" 2>/dev/null; then
            sed -n -e 's/.*\(OBS|.*\)/\1/p' "$usbtmp" 2>/dev/null >> "$obs" || true
            awk '!seen[$0]++' "$obs" > "$obs.tmp" && mv -f "$obs.tmp" "$obs"
            n=$(grep -acE '^OBS\|' "$obs" 2>/dev/null || true)
            end=$(grep -acE '^OBS\|end' "$obs" 2>/dev/null || true)
        fi
        rm -f "$usbtmp"
    fi
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
