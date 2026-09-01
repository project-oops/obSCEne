#!/bin/bash
# Paths are derived from this script's own location rather than hardcoded, so the
# collection works wherever it is cloned. `$OOPS` is the parent holding all four projects.
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
OOPS="$(cd "$REPO/.." && pwd)"
export PATH="$HOME/.cargo/bin:$PATH" CARGO_TARGET_DIR="$HOME/obs-tool-target"
cd "$REPO"
BIN="$HOME/obs-tool-target/debug/obscene-tool"
"$BIN" hw pull /system/common/lib/libSceVideoOut.sprx --into "$HOME/libSceVideoOut.sprx" 2>&1 | tail -1
echo "size: $(stat -c %s "$HOME/libSceVideoOut.sprx" 2>/dev/null)"
for f in sceVideoOutOpen sceVideoOutSetBufferAttribute sceVideoOutRegisterBuffers sceVideoOutSetFlipRate sceVideoOutSubmitFlip sceVideoOutGetResolutionStatus; do
  nid=$("$BIN" nid "$f" 2>/dev/null | grep encoded | grep -oE '[A-Za-z0-9+_-]{11}' | head -1)
  printf '%-34s %s\n' "$f" "$nid"
done
