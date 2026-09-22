#!/usr/bin/env bash
# pull-log.sh - pull the latest obSCEne report/log from the target console via prosperous FTP.
#
# Usage:
#   ./bin/obscene pull-log [destination] [OPTIONS]
#   ./bin/obscene pull     [destination] [OPTIONS]
#
# Options:
#   --into <dest>       Explicit local destination path (default: reports/obscene-report.txt)
#   --remote <path>     Explicit remote path to pull (bypasses auto-discovery)
#   --name <target>     Target console name registered in prosperous (default: active target)
#   --archive           Also save a timestamped copy under reports/ (default: on)
#   -h, --help          Show this help text
#
# Auto-discovery checks writable sink candidate directories on the target:
#   /mnt/usb0/obscene, /mnt/usb1/obscene, /data/homebrew/PPSA90000, /data/obscene, /data
# and selects the newest timestamped archive (report-<timestamp>.txt) or report.txt.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
cd "$REPO"

# Tool resolution (first-party tooling per AGENTS.md §1)
TOOL=""
for cand in \
    "$REPO/tool/target/release/obscene-tool" \
    "$REPO/tool/target/release/obscene-tool.exe" \
    "$REPO/tool/target/debug/obscene-tool" \
    "$REPO/tool/target/debug/obscene-tool.exe"; do
    if [ -x "$cand" ]; then
        TOOL="$cand"
        break
    fi
done

if [ -z "$TOOL" ]; then
    echo "pull-log: building obscene-tool..." >&2
    ( cd "$REPO/tool" && cargo build --release --quiet )
    TOOL="$REPO/tool/target/release/obscene-tool"
    [ -x "$TOOL.exe" ] && TOOL="$TOOL.exe"
fi

# Argument parsing
INTO=""
REMOTE_PATH=""
TARGET_NAME=""
ARCHIVE=1

while [ $# -gt 0 ]; do
    case "$1" in
        --into)
            INTO="$2"
            shift 2
            ;;
        --remote)
            REMOTE_PATH="$2"
            shift 2
            ;;
        --name|-n)
            TARGET_NAME="$2"
            shift 2
            ;;
        --no-archive)
            ARCHIVE=0
            shift
            ;;
        --archive)
            ARCHIVE=1
            shift
            ;;
        -h|--help)
            sed -n '2,17p' "$0" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        -*)
            echo "pull-log: unrecognized option '$1' (try --help)" >&2
            exit 1
            ;;
        *)
            if [ -z "$INTO" ]; then
                INTO="$1"
            else
                echo "pull-log: unexpected extra argument '$1'" >&2
                exit 1
            fi
            shift
            ;;
    esac
done

NAME_ARGS=()
if [ -n "$TARGET_NAME" ]; then
    NAME_ARGS=(--name "$TARGET_NAME")
fi

# Ensure reports output directory exists
mkdir -p "$REPO/reports"
DEFAULT_INTO="$REPO/reports/obscene-report.txt"
DEST="${INTO:-$DEFAULT_INTO}"

FOUND_REMOTE=""
FOUND_TS_NAME=""

if [ -n "$REMOTE_PATH" ]; then
    FOUND_REMOTE="$REMOTE_PATH"
else
    # Auto-discovery candidates matching sink.c writable search paths
    CANDIDATE_DIRS=(
        "/mnt/usb0/obscene"
        "/mnt/usb1/obscene"
        "/data/homebrew/PPSA90000"
        "/data/obscene"
        "/data"
    )

    for dir in "${CANDIDATE_DIRS[@]}"; do
        listing="$("$TOOL" hw ls "$dir" "${NAME_ARGS[@]}" 2>/dev/null || true)"
        if [ -z "$listing" ]; then
            continue
        fi

        # Find newest timestamped report (report-<timestamp>.txt)
        ts_file=$(echo "$listing" | awk '{print $NF}' | grep -E '^report-[0-9]+\.txt$' | sort -t- -k2 -n | tail -1 || true)
        if [ -n "$ts_file" ]; then
            FOUND_REMOTE="$dir/$ts_file"
            FOUND_TS_NAME="$ts_file"
            break
        fi

        # Fallback to plain report.txt or obscene-report.txt in this candidate directory
        if echo "$listing" | grep -qE '[[:space:]]report\.txt$'; then
            FOUND_REMOTE="$dir/report.txt"
            break
        elif echo "$listing" | grep -qE '[[:space:]]obscene-report\.txt$'; then
            FOUND_REMOTE="$dir/obscene-report.txt"
            break
        fi
    done
fi

if [ -z "$FOUND_REMOTE" ]; then
    echo "pull-log: no report file found on target via FTP across candidates:" >&2
    echo "  /mnt/usb0/obscene, /mnt/usb1/obscene, /data/homebrew/PPSA90000, /data/obscene, /data" >&2
    echo "  Is ftpsrv (:2121) running? Check with: ./bin/obscene check or pros check" >&2
    exit 1
fi

echo "pull-log: pulling $FOUND_REMOTE via prosperous FTP..."
"$TOOL" hw pull "$FOUND_REMOTE" --into "$DEST" "${NAME_ARGS[@]}"

# If a timestamped archive was discovered and user didn't specify a custom destination,
# also keep the timestamped archive beside obscene-report.txt
if [ "$ARCHIVE" = 1 ] && [ -n "$FOUND_TS_NAME" ]; then
    ARCHIVE_DEST="$REPO/reports/$FOUND_TS_NAME"
    if [ "$DEST" != "$ARCHIVE_DEST" ]; then
        cp -f "$DEST" "$ARCHIVE_DEST"
        echo "pull-log: archived copy saved to $ARCHIVE_DEST"
    fi
fi

# Print summary
if [ -f "$DEST" ]; then
    bytes=$(wc -c < "$DEST" | tr -d ' ')
    obs_lines=$(grep -cE '^OBS\|' "$DEST" || true)
    passes=$(grep -cE '^OBS\|res\|.*\|pass\|' "$DEST" || true)
    skips=$(grep -cE '^OBS\|res\|.*\|skip\|' "$DEST" || true)
    fails=$(grep -cE '^OBS\|res\|.*\|fail\|' "$DEST" || true)
    measures=$(grep -cE '^OBS\|measure\|' "$DEST" || true)
    echo "pull-log: complete ($bytes bytes, $obs_lines OBS records: $passes pass, $fails fail, $skips skip, $measures measurements)"
    echo "pull-log: verify with: ./bin/obscene verify $DEST"
fi
