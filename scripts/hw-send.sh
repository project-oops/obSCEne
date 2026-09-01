#!/bin/bash
# Compile a payload with the crt0/entry given, send it, show matching output + signal.
# Paths are derived from this script's own location rather than hardcoded, so the
# collection works wherever it is cloned. `$OOPS` is the parent holding all four projects.
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
OOPS="$(cd "$REPO/.." && pwd)"
export PATH="$HOME/.cargo/bin:$PATH"
cd "$REPO"
BIN="$HOME/obs-tool-target/debug/obscene-tool"
src="$1"; entry="${2:-obscene_start}"; grepfor="${3:-OBS}"
out="$HOME/obs-hw/$(basename "$src" .c).elf"
clang -std=c11 -Wall -Wextra -target x86_64-unknown-freebsd -ffreestanding -fno-builtin \
  -nostdlib -fPIC -fno-stack-protector -fuse-ld=lld -shared -Wl,-e,"$entry" \
  -Wl,--unresolved-symbols=ignore-all -Wl,-z,noexecstack \
  -Wl,-z,max-page-size=0x4000 -Wl,-z,common-page-size=0x4000 \
  -o "$out" "$src" 2>&1 | head -5
[ -f "$out" ] || { echo "BUILD FAILED"; exit 1; }
echo "built: $(stat -c %s "$out") bytes"
( $BIN hw logs --seconds 12 > "$HOME/klog-hs.txt" 2>&1 ) & L=$!
sleep 3
$BIN hw send "$out" --seconds 8 2>&1 | grep -iE "$grepfor" | head -20
wait $L
echo "signal: $(grep -oE 'signo: 0x[0-9a-f]+' "$HOME/klog-hs.txt" | tail -1)"
