#!/bin/bash
export PATH="$HOME/.cargo/bin:$PATH"
BIN="$HOME/obs-tool-target/debug/obscene-tool"
elf="$1"
( $BIN hw logs --seconds 16 > "$HOME/klog-ks.txt" 2>&1 ) & L=$!
sleep 4
$BIN hw send "$elf" --seconds 6 >/dev/null 2>&1
wait $L
echo "signo: $(grep -oE 'signo: 0x[0-9a-f]+' "$HOME/klog-ks.txt" | tail -1)"
echo "PPRBUG: $(grep 'PPRBUG' "$HOME/klog-ks.txt" | head -1)"
echo "backtrace addr: $(grep -A2 'backtrace:' "$HOME/klog-ks.txt" | grep -oE '^# [0-9a-f]{16}' | head -2 | tr '\n' ' ')"
