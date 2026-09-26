#!/bin/bash
# Runs verify.sh inside WSL and streams each gate, with the second it started, to
# $HOME/verify-progress.txt, so a slow run can be told apart from a hung one. The full output
# goes to $HOME/verify.txt.
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PATH="$HOME/.cargo/bin:$PATH" CARGO_TARGET_DIR="$HOME/obs-tool-target"
cd "$REPO" || exit
p="$HOME/verify-progress.txt"
: > "$p"
start=$(date +%s)
bash scripts/verify.sh BUILD="$HOME/obs" 2>&1 | while IFS= read -r l; do
    case "$l" in
        "==="*|*FAILED*|"verify:"*) printf '[%4ss] %s\n' "$(( $(date +%s) - start ))" "$l" >> "$p" ;;
    esac
    printf '%s\n' "$l"
done > "$HOME/verify.txt"
rc=${PIPESTATUS[0]}
printf '[%4ss] EXIT=%s\n' "$(( $(date +%s) - start ))" "$rc" >> "$p"
exit "$rc"
