#!/bin/bash
export PATH="$HOME/.cargo/bin:$PATH"
BIN="$HOME/obs-tool-target/debug/obscene-tool"
for n in klogsrv shsrv; do
  f=$(ls $HOME/${n}*.elf 2>/dev/null | head -1)
  if [ -z "$f" ]; then
    r=$("$BIN" hw ls /data/pldmgr/payloads/$n 2>&1 | grep -oE "${n}[a-zA-Z0-9_.-]*\.elf" | head -1)
    f="$HOME/$r"; "$BIN" hw pull "/data/pldmgr/payloads/$n/$r" --into "$f" >/dev/null 2>&1
  fi
  echo "--- sending $(basename $f) ---"
  "$BIN" hw send "$f" --seconds 12 2>&1 | grep -iE "Serving|listen|error" | head -2
done
echo "=== services ==="
"$BIN" hw check 2>&1 | head -7
