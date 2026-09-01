#!/bin/sh
# Runs a built module under Kyty and extracts the report.
#
# Kyty is the second loader this suite runs against, and it matters for a reason beyond
# coverage: it is an independent implementation, so where it and shadPS4 disagree about the
# platform, one of them is wrong and the disagreement is itself the finding.
#
# It is also the better source for *absence*. Kyty patches each unresolved import
# individually and names it, so its log lists what it does not implement - 238 functions -
# where shadPS4 stubs everything and so reports none. `obscene-tool unresolved` turns that
# log into names.
#
# # Its output was thought to be uncapturable, and is not
#
# Kyty's console output is invisible to a parent process, which is why obSCEne learned to
# draw its report to the screen. `--printf-direction File` removes the problem entirely.
# The drawn report is still worth having - it will matter on hardware - but it was built to
# route around a command-line flag nobody had looked for.
#
# # The binary is newer than the repository's default branch
#
# Kyty's checked-out source describes a Lua configuration interface. The build here is
# 0.2.2 and takes arguments. Read the clone to understand the loader; ask the binary how to
# drive it, which it explains when given nothing.
# Paths are derived from this script's own location rather than hardcoded, so the
# collection works wherever it is cloned. `$OOPS` is the parent holding all four projects.
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
OOPS="$(cd "$REPO/.." && pwd)"
EMULATORS="${EMULATORS:-$(cd "$OOPS/.." && pwd)/emulators}"

set -e

EMULATOR="${EMULATOR:-$EMULATORS/kyty/kyty_emulator.exe}"
OUT="${OUT:-reports/kyty.txt}"
TIMEOUT="${TIMEOUT:-240}"
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

while [ $# -gt 0 ]; do
    case "$1" in
        --emulator) EMULATOR="$2"; shift 2 ;;
        --out) OUT="$2"; shift 2 ;;
        --timeout) TIMEOUT="$2"; shift 2 ;;
        *) echo "unknown option: $1" >&2; exit 2 ;;
    esac
done

[ -f "$EMULATOR" ] || { echo "no Kyty at $EMULATOR - see docs/EMULATORS.md" >&2; exit 1; }

# Kyty is given a directory or an ELF; a stable staging directory keeps the log where the
# NID reader can find it afterwards.
stage="${TMPDIR:-/tmp}/obscene-kyty"
mkdir -p "$stage"
module="$stage/main.elf"
printf_log="$stage/_kyty.txt"
rm -f "$printf_log"

local_target="$module"
command -v cygpath >/dev/null 2>&1 && local_target=$(cygpath -w "$module")
# See scripts/wsl.sh. The `|| true` and the test below are kept deliberately - testing for
# the file rather than trusting an exit code was the right shape under multipass and still is.
vm transfer "${VM}:${VM_MODULE}" "$local_target" || true
[ -f "$module" ] || { echo "could not fetch $VM_MODULE from $VM" >&2; exit 1; }

win_module="$module"
win_log="$printf_log"
if command -v cygpath >/dev/null 2>&1; then
    win_module=$(cygpath -w "$module")
    win_log=$(cygpath -w "$printf_log")
fi

stdout="$stage/stdout.txt"
timeout "$TIMEOUT" "$EMULATOR" \
    --game "$win_module" \
    --printf-direction File \
    --printf-output-file "$win_log" \
    --vulkan-validation false \
    --shader-validation false \
    --shader-log-direction Silent \
    --command-buffer-dump false \
    >"$stdout" 2>&1 || true

mkdir -p "$(dirname "$OUT")"
# The report may arrive by either route, so both are read.
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
cat "$printf_log" "$stdout" 2>/dev/null | tr -d '\000' | grep -o 'OBS|.*' | sort -u > "$OUT" || true

echo "records:     $(wc -l < "$OUT" | tr -d ' ')"
echo "report:      $OUT"
echo "kyty log:    $printf_log"
echo
echo "what it does not implement:"
echo "  obscene-tool unresolved '$printf_log'"
