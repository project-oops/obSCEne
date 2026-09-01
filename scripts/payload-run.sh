#!/bin/bash
# Build obSCEne's payload and run it on the console through elfldr, capturing what it wrote.
#
# The payload is the unsandboxed shape - a plain ELF, no SELF and no package. elfldr maps it and
# runs it, and it reaches libkernel by base+vaddr rather than by bound imports. It reports on two
# channels: the send socket when its net sink connects back (a run that reached that far), and the
# system log (D233) otherwise - which is also where a crash leaves its fatal-signal lines. This
# captures both across the run and reads whichever spoke, so a running payload's report and a
# crash are told apart rather than both reading as "nothing arrived".
#
# Everything here connects *out* to the console, so unlike the package deploy there is no Windows
# half: it runs entirely in WSL, and re-enters WSL itself when started from a Windows shell.
#
#   --title ID     launch title on console before injecting (e.g. PPSA02664)
#   --seconds N    run/capture window                       (default 90)
#   --into FILE    where to write the captured log           (default reports/hardware/payload-klog.txt)
#   --build-only   build the payload and stop
#   --deploy-only  skip build and send prebuilt binary
#   --name NAME    which registered console, if several
#   -- MAKEFLAGS   anything after `--` goes to `make payload` (GEN=5, CORPUS=0, ...)
set -e
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"

# Re-enter WSL if not already there - Git Bash has no clang. Guarded by an env var so a distro
# that somehow reports itself wrongly loops once and stops rather than forever. This is why the
# work lives in a file: `wsl.exe -- bash -lc '...'` mangles inline args and shell variables under
# Git Bash, so a script is the only thing that runs the same both ways (repository CLAUDE.md).
if ! grep -qi microsoft /proc/version 2>/dev/null; then
    [ -n "$OBS_PAYLOAD_REENTERED" ] && { echo "payload-run.sh: re-entered WSL and still not in WSL" >&2; exit 1; }
    linux="$(printf '%s' "$HERE/payload-run.sh" | sed 's|^/\([a-zA-Z]\)/|/mnt/\1/|')"
    echo "payload-run.sh: not in WSL - re-entering ${WSL_DISTRO:-Ubuntu}"
    exec wsl.exe -d "${WSL_DISTRO:-Ubuntu}" -- env OBS_PAYLOAD_REENTERED=1 bash "$linux" "$@"
fi

seconds=90
into=""
send=1
do_build=1
use_injector=0
title=""
name_arg=()
make_flags=()
while [ $# -gt 0 ]; do
    case "$1" in
        --title)      title="$2"; shift 2 ;;
        --seconds)    seconds="$2"; shift 2 ;;
        --into)       into="$2"; shift 2 ;;
        --build-only) send=0; shift ;;
        --deploy-only|--send-only) do_build=0; shift ;;
        --injector|--inject) use_injector=1; shift ;;
        --name)       name_arg=(--name "$2"); shift 2 ;;
        --)           shift; make_flags=("$@"); break ;;
        -h|--help)    sed -n '2,19p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "payload-run.sh: unknown option $1 (try --help)" >&2; exit 2 ;;
    esac
done

if [ -z "$into" ]; then
    if [ "$use_injector" = 1 ]; then
        into="$REPO/reports/hardware/injector-klog.txt"
    else
        into="$REPO/reports/hardware/payload-klog.txt"
    fi
fi

export PATH="$HOME/.cargo/bin:$PATH"
export CARGO_TARGET_DIR="$HOME/obs-tool-target"
BUILD="${BUILD:-$HOME/obs}"

if [ "$use_injector" = 1 ]; then
    if [ "$do_build" = 1 ]; then
        echo "=== build payload and injector (make payload injector HARDWARE=1) ==="
        rm -f "$BUILD/obscene-injector.elf" "$BUILD/obscene-payload.elf"
        make -C "$REPO" payload injector HARDWARE=1 BUILD="$BUILD" "${make_flags[@]}"
    else
        echo "=== skipping build (--deploy-only) ==="
    fi
    elf="$BUILD/obscene-injector.elf"
    [ -f "$elf" ] || { echo "payload-run.sh: injector binary not found at $elf" >&2; exit 1; }
    pelf="$BUILD/obscene-payload.elf"
    [ -f "$pelf" ] || pelf="$BUILD/obscene.elf"
    echo "using injector: $elf ($(stat -c %s "$elf") bytes)"
    [ -f "$pelf" ] && echo "using payload:  $pelf ($(stat -c %s "$pelf") bytes)"
else
    if [ "$do_build" = 1 ]; then
        echo "=== build payload (make payload HARDWARE=1) ==="
        rm -f "$BUILD/obscene-payload.elf" "$BUILD/obscene.elf"
        make -C "$REPO" payload HARDWARE=1 BUILD="$BUILD" "${make_flags[@]}"
    else
        echo "=== skipping build (--deploy-only) ==="
    fi
    elf="$BUILD/obscene-payload.elf"
    [ -f "$elf" ] || elf="$BUILD/obscene.elf"
    [ -f "$elf" ] || { echo "payload-run.sh: payload binary not found at $elf" >&2; exit 1; }
    echo "using: $elf ($(stat -c %s "$elf") bytes)"
fi

[ "$send" = 1 ] || { echo "stopped before the send (--build-only)."; exit 0; }

if [ -x "$REPO/tool/target/release/obscene-tool.exe" ]; then
    tool="$REPO/tool/target/release/obscene-tool.exe"
elif [ -x "$REPO/target/release/obscene-tool.exe" ]; then
    tool="$REPO/target/release/obscene-tool.exe"
elif [ -x "$CARGO_TARGET_DIR/release/obscene-tool" ]; then
    tool="$CARGO_TARGET_DIR/release/obscene-tool"
elif [ -x "$REPO/tool/target/release/obscene-tool" ]; then
    tool="$REPO/tool/target/release/obscene-tool"
else
    tool="$CARGO_TARGET_DIR/release/obscene-tool"
    ( cd "$REPO/tool" && cargo build --release --quiet ) || true
fi

mkdir -p "$(dirname "$into")"
klog="$into.klog"

if [ -n "$title" ]; then
    title_upper="$(echo "$title" | tr '[:lower:]' '[:upper:]')"
    echo "=== launching title $title_upper on console ==="
    if [ -x "$REPO/../prosperous/target/release/pros.exe" ]; then
        "$REPO/../prosperous/target/release/pros.exe" launch "$title_upper" "${name_arg[@]}" || true
        pros launch "$title_upper" "${name_arg[@]}" || true
    else
    fi
    echo "waiting 2s for title to initialize..."
    sleep 2
fi

echo
echo "=== run on console via elfldr + capture both channels (${seconds}s) ==="
# A payload reports on **two** channels and which one carries it says something. When it runs,
# its net sink connects back and the report arrives on the send socket ($into). When it cannot -
# a crash before the sink is up, or a net stack that never comes up - it falls back to the system
# log ($klog, D233), which is also where the kernel writes the fatal-signal lines a crash leaves.
# So capture the log across the run *and* keep the full socket output, and read whichever spoke.
# The log capture starts first, so nothing emitted at startup is lost.
"$tool" hw logs --seconds "$((seconds + 10))" "${name_arg[@]}" > "$klog" 2>&1 &
cap=$!
# Prepare elf path for tool (handle Windows .exe path if running on WSL)
elf_arg="$elf"
if [[ "$tool" == *.exe ]]; then
    mkdir -p "$REPO/build"
    cp -f "$elf" "$REPO/build/$(basename "$elf")"
    elf_arg="$(wslpath -w "$REPO/build/$(basename "$elf")")"
fi

# Send the payload in background and monitor klog in real time
"$tool" hw send "$elf_arg" --seconds "$seconds" "${name_arg[@]}" 2>&1 | tr -d '\r' > "$into" &
send_pid=$!

# Real-time poll loop: detect completion (OBS|end, OBS|tally) or fatal crash immediately
max_iter=$((seconds + 5))
i=0
while [ "$i" -lt "$max_iter" ]; do
    sleep 1
    i=$((i + 1))
    if grep -qaE 'fatal signal|signo:' "$klog" 2>/dev/null; then
        sleep 1
        break
    fi
    if grep -qaE '^OBS\|(end|tally)\|' "$klog" "$into" 2>/dev/null; then
        sleep 1
        break
    fi
    if [ "$use_injector" = 0 ] && ! kill -0 "$send_pid" 2>/dev/null; then
        sleep 1
        break
    fi
done

kill -9 "$send_pid" 2>/dev/null || true
kill -9 "$cap" 2>/dev/null || true
wait "$send_pid" 2>/dev/null || true
wait "$cap" 2>/dev/null || true

echo
# `grep -c` prints `0` *and* exits non-zero on no match: `|| echo 0` would append a second `0`
# (`0\n0`, which no arithmetic accepts), and under `set -e` a bare `n=$(grep -c ...)` inherits the
# non-zero and kills the script. `|| true` keeps grep's printed `0` and drops the status; `:-0`
# only covers a missing file. (Both traps are in oops-hwsweep.sh - this reproduced them.)
ns=$(grep -acE '^OBS\|' "$into" 2>/dev/null || true); ns=${ns:-0}
nk=$(grep -acE '^OBS\|' "$klog" 2>/dev/null || true); nk=${nk:-0}
if [ "$ns" -gt 0 ]; then
    echo "payload ran: $ns OBS record(s), on the socket -> $into"
    grep -aE '^OBS\|(meta|build|tally|end|sink|display)\|' "$into" | head
elif [ "$nk" -gt 0 ]; then
    echo "payload ran: $nk OBS record(s), on the system log -> $klog (the net sink did not connect back; D233)"
    grep -aE '^OBS\|(meta|build|tally|end|sink|display)\|' "$klog" | head
elif grep -qaE 'fatal signal|signo:' "$klog" 2>/dev/null; then
    echo "no records - the payload took a fatal signal before it could report ($klog):"
    # The fault-address line names what it touched (e.g. a library base + offset) - why, not just
    # that it died.
    grep -aE '# fault address:|# rip:|signo:|fatal signal' "$klog" | tail -5
else
    echo "no records and no crash signal - check $into (socket) and $klog (system log)."
fi
