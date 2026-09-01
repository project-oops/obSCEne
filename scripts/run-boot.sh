#!/bin/bash
# Paths are derived from this script's own location rather than hardcoded, so the
# collection works wherever it is cloned. `$OOPS` is the parent holding all four projects.
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
OOPS="$(cd "$REPO/.." && pwd)"
export PATH="$HOME/.cargo/bin:$PATH" CARGO_TARGET_DIR="$HOME/obs-tool-target"
cd "$REPO"
T="cargo run --manifest-path tool/Cargo.toml -q --"
src="${1:-src/experiments/boot.c}"
clang -std=c11 -Wall -Wextra -target x86_64-unknown-freebsd -ffreestanding -fno-builtin \
  -nostdlib -fPIC -fno-stack-protector -fuse-ld=lld -shared -Wl,-e,obscene_start \
  -Wl,-z,noexecstack -Wl,-z,max-page-size=0x4000 -Wl,-z,common-page-size=0x4000 \
  -o "$HOME/obs-hw/boot.elf" "$src" 2>&1 | head -3
[ -f "$HOME/obs-hw/boot.elf" ] || { echo "BUILD FAILED"; exit 1; }
( $T hw logs --seconds 14 > "$HOME/klog-boot.txt" 2>&1 ) & L=$!
sleep 3
echo "=== SOCKET OUTPUT ==="
$T hw send "$HOME/obs-hw/boot.elf" --seconds 9 2>&1 | grep -iE "OBSCENE|hardware|FW12" | head -3
wait $L
echo "=== SIGNAL ==="
grep -oE "signo: 0x[0-9a-f]+" "$HOME/klog-boot.txt" | tail -1
echo "=== KLOG for OBSCENE text ==="
grep -iE "OBSCENE|FW12" "$HOME/klog-boot.txt" | head -2
