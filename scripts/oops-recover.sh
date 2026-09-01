#!/bin/bash
# Read-only recovery sweep: what a real console recorded, after a crash or a reboot.
#
#     ./bin/obscene recover
#
# Pulls nothing but evidence and changes nothing on the console. Run it the moment the console is
# back, before any fresh launch - a launch costs an hour of jailbreak, and everything here is
# already on the SSD or in a service that is already up.
#
# Three questions, in order of how coarse they are:
#
#   1. did the eboot run at all?      obSCEne's boot log, written a syscall per note (obs_boot_note)
#                                     before each step, so its LAST line names what was being
#                                     attempted when the machine stopped. It exists even for a build
#                                     that serves on start and never opens the report sink.
#   2. what did the run report?       obSCEne's report file, wherever src/sink.c managed to land it.
#                                     Note this is the *fallback* path: a packaged run's report
#                                     leaves the sandbox on the system log (D233, `./bin/obscene
#                                     report`), and the disk file is normally sealed inside the
#                                     sandbox - but a crash can leave one somewhere reachable, so
#                                     it costs nothing to look.
#   3. did the console itself crash?  the console's own crash reports and core dumps, found by
#                                     recursing the filesystem - these outlive the reboot the way
#                                     the kernel ring buffer does not.
#
# "absent" and "UNREADABLE" are kept apart throughout, because conflating them is how a missing
# file and a downed service became one sentence, and a boot log that may well exist read three
# times as proof the eboot never ran. "absent" means the console was asked and said no;
# "UNREADABLE" means the service needed to ask was down.
set -u
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
export PATH="$HOME/.cargo/bin:$PATH"
BIN="$HOME/obs-tool-target/debug/obscene-tool"
[ -x "$BIN" ] || BIN="$REPO/tool/target/release/obscene-tool"
[ -x "$BIN" ] || { echo "recover: no obscene-tool built (looked in ~/obs-tool-target and tool/target/release)" >&2; exit 1; }
OUT="${OUT:-$HOME/obs-recovered}"
mkdir -p "$OUT"

echo "=== services (what can be asked at all) ==="
services="$("$BIN" hw check 2>&1)" || true
printf '%s\n' "$services" | head -7
ftp_up=0; printf '%s\n' "$services" | grep -qE "up +ftpsrv" && ftp_up=1
sh_up=0;  printf '%s\n' "$services" | grep -qE "up +shsrv"  && sh_up=1
echo

# ---------------------------------------------------------------- 1. did the eboot run?
echo "=== obSCEne's boot log (last line names what it was attempting) ==="
if [ "$ftp_up" = 0 ]; then
    echo "  UNREADABLE: ftpsrv is down, so nothing below was actually checked. Not evidence"
    echo "  about the eboot either way - bring ftpsrv up and re-run."
else
    bootfound=0
    for p in /data/obscene-boot.txt /download0/obscene-boot.txt /mnt/usb0/obscene-boot.txt \
             /app0/obscene-boot.txt; do
        dir="${p%/*}"; name="${p##*/}"
        listing="$("$BIN" hw ls "$dir" 2>/dev/null)" || { echo "  UNREADABLE $p (cannot list $dir)"; continue; }
        if printf '%s\n' "$listing" | grep -q "$name"; then
            echo "  FOUND $p"
            "$BIN" hw pull "$p" --into "$OUT/$(echo "$p" | tr '/' '_')" >/dev/null 2>&1
            sed 's/^/    | /' "$OUT/$(echo "$p" | tr '/' '_')" 2>/dev/null
            bootfound=1
        else
            echo "  absent $p"
        fi
    done
    [ "$bootfound" = 0 ] && echo "  no boot log anywhere ftpsrv can see: the eboot's first instruction never ran"
fi
echo

# ---------------------------------------------------------------- 2. what did the run report?
# The sink candidates, in the order src/sink.c tries them. `/app0` is the cwd case and normally
# read-only, but it costs nothing to look.
echo "=== obSCEne's report file, wherever it landed (fallback to the klog path) ==="
if [ "$ftp_up" = 0 ]; then
    echo "  UNREADABLE: ftpsrv down"
else
    found=0
    for p in /data/obscene-report.txt /download0/obscene-report.txt /mnt/usb0/obscene-report.txt \
             /app0/obscene-report.txt; do
        dir="${p%/*}"; name="${p##*/}"
        listing="$("$BIN" hw ls "$dir" 2>/dev/null)" || { echo "  UNREADABLE $p (cannot list $dir)"; continue; }
        if printf '%s\n' "$listing" | grep -q "$name"; then
            echo "  FOUND $p"
            "$BIN" hw pull "$p" --into "$OUT/$(echo "$p" | tr '/' '_')" >/dev/null 2>&1
            found=1
        else
            echo "  absent $p"
        fi
    done
    [ "$found" = 0 ] && echo "  no report file reachable: a packaged run's report is on the system log instead (D233)"
fi
echo

# ---------------------------------------------------------------- 3. did the console itself crash?
# `/app0` and `/download0` are sandbox-relative and do not exist outside a title, so they are not
# searched here. The console's `find` takes a path and recurses but takes **no predicates** -
# `-name` and the rest come back as `Unknown option` or an empty listing and a clean exit, which
# is the worst available failure: a search that greps directories for a pattern the console never
# applied reads exactly like a console with nothing to find. So the recursion happens there and the
# matching happens here, where the predicates work.
echo "=== crash reports and core dumps (recursive) ==="
if [ "$sh_up" = 0 ]; then
    echo "  UNREADABLE: shsrv is down, and recursive find is the only way to search properly"
else
    for root in /user /data /system_data /preinst; do
        echo "  --- $root ---"
        listing="$("$BIN" hw sh "find $root" 2>&1)"
        if printf '%s' "$listing" | grep -q "Unknown option\|No path specified"; then
            echo "  UNREADABLE: the console refused 'find $root'"
            continue
        fi
        hits="$(printf '%s\n' "$listing" \
            | grep -viE "^/\$|No such file|Permission denied|^\s*$" \
            | grep -iE 'crash|core|dump|OBSC|\.mdmp|panic' \
            | head -25)"
        if [ -z "$hits" ]; then
            count="$(printf '%s\n' "$listing" | grep -cvE "^/\$|^\s*$")"
            echo "  absent - nothing matching in $count paths the console listed"
        else
            printf '%s\n' "$hits"
        fi
    done
fi
echo

# ---------------------------------------------------------------- is the title still registered?
echo "=== is the title still registered after the crash? ==="
if [ "$ftp_up" = 1 ]; then
    "$BIN" hw ls /user/app 2>/dev/null | grep -i OBSC || echo "  OBSC00001 not in /user/app"
else
    echo "  UNREADABLE: ftpsrv down"
fi
echo "anything pulled is in $OUT"
