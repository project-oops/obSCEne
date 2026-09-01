#!/bin/bash
# Paths are derived from this script's own location rather than hardcoded, so the
# collection works wherever it is cloned. `$OOPS` is the parent holding all four projects.
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
OOPS="$(cd "$REPO/.." && pwd)"
export PATH="$HOME/.cargo/bin:$PATH" CARGO_TARGET_DIR="$HOME/obs-tool-target"
cd "$REPO"
BIN="$HOME/obs-tool-target/debug/obscene-tool"
src="${1:-src/experiments/hwreport.c}"
clang -std=c11 -Wall -Wextra -target x86_64-unknown-freebsd -ffreestanding -fno-builtin \
  -nostdlib -fPIC -fno-stack-protector -fuse-ld=lld -shared -Wl,-e,obscene_start \
  -Wl,-z,noexecstack -Wl,-z,max-page-size=0x4000 -Wl,-z,common-page-size=0x4000 \
  -o "$HOME/obs-hw/report.elf" "$src" 2>&1 | head -3
[ -f "$HOME/obs-hw/report.elf" ] || { echo "BUILD FAILED"; exit 1; }
$BIN hw send "$HOME/obs-hw/report.elf" --seconds 9 2>&1 | grep -E "^OBS\|" > "$HOME/hwreport.txt"
echo "=== FULL HARDWARE REPORT ($(wc -l < "$HOME/hwreport.txt") records) ==="
cat "$HOME/hwreport.txt"
cp "$HOME/hwreport.txt" $REPO/reports/hardware/first-report.txt
