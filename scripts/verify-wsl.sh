#!/bin/bash
# Run verify.sh inside WSL with per-gate progress, so a long run can be watched rather than
# waited on blind.
#
# verify.sh only prints its verdict at the end, so a run that is merely slow is
# indistinguishable from one that has hung - which cost a 50-minute wait and three background
# tasks that could not be told apart from finished ones. This streams each gate with the time
# it started, to $HOME/verify-progress.txt.
# Paths are derived from this script's own location rather than hardcoded, so the
# collection works wherever it is cloned. `$OOPS` is the parent holding all four projects.
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
OOPS="$(cd "$REPO/.." && pwd)"
export PATH="$HOME/.cargo/bin:$PATH" CARGO_TARGET_DIR="$HOME/obs-tool-target"
cd "$REPO"
p="$HOME/verify-progress.txt"
: > "$p"
start=$(date +%s)
bash scripts/verify.sh BUILD="$HOME/obs" 2>&1 | while IFS= read -r l; do
    case "$l" in
        "==="*|*FAILED*|"verify:"*) printf '[%4ss] %s\n' "$(( $(date +%s) - start ))" "$l" >> "$p" ;;
    esac
    printf '%s\n' "$l"
done > "$HOME/verify.txt"
rc=$?
printf '[%4ss] EXIT=%s\n' "$(( $(date +%s) - start ))" "$rc" >> "$p"
