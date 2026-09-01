#!/bin/bash
# Build the minimal package: a variant of src/min.c, under a title id of your choosing.
#
#     wsl.exe -d Ubuntu -- bash <OOPS>/obscene/scripts/oops-minbuild.sh
#     MIN=-DOBSCENE_MIN_FILE wsl.exe -d Ubuntu -- bash .../oops-minbuild.sh
#     CONTENT_ID=IV0002-OBSC00003_00-STOREUPD00000000 wsl.exe -d Ubuntu -- bash .../oops-minbuild.sh
#
# Three knobs, and each exists because leaving it out has cost a run:
#
#   PAD_BYTES   the inner filesystem must exceed a size in (720896, 1769472]. Without padding
#               `pkg-min` lands exactly on 720896 and the console refuses the mount long before
#               the eboot is looked at - a failure that looks nothing like a size problem.
#   MIN         which variant of src/min.c. The default writes to the kernel log and then spins,
#               which is what a healthy boot looks like: the title stays in state RUN and its
#               message is in `hw logs`. `-DOBSCENE_MIN_FILE` returns instead, and returning from
#               the entry faults - and note /data is NOT writable from inside a game sandbox.
#   CONTENT_ID  carries the title id with it (see build-pkg.sh). A crashed process the console
#               will not reap holds its title id and every install against it is refused after
#               the header; a fresh id routes around that without a reboot. (D223)
#
# The package is copied to the repository root because `hw install` must run from **Windows**:
# the console connects *in* to fetch it, and WSL's NAT address is not one it can reach.
# Paths are derived from this script's own location rather than hardcoded, so the
# collection works wherever it is cloned. `$OOPS` is the parent holding all four projects.
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
OOPS="$(cd "$REPO/.." && pwd)"

set -e
export PATH="$HOME/.cargo/bin:$PATH" CARGO_TARGET_DIR="$HOME/obs-tool-target"
cd "$OOPS/obscene"
PAD_BYTES="${PAD_BYTES:-262144}" \
  MIN_DEFINES="${MIN:--DOBSCENE_MIN_DEBUG_OUT}" \
  CONTENT_ID="${CONTENT_ID:-IV0002-OBSC00002_00-STOREUPD00000000}" \
  make pkg-min BUILD="$HOME/obs-min" GEN="${GEN:-5}" HARDWARE=1 2>&1 \
  | grep -E "keyed to|param.sfo|reproduces|does not reproduce|symbols from|wrote|warning|error" || true
cp "$HOME/obs-min/obscene.pkg" "$OOPS/obscene/build-min.pkg"
echo
echo "build-min.pkg ready. From Windows, in tool/:"
echo "  ./target-win/debug/obscene-tool.exe hw install $OOPS/obscene/build-min.pkg"
echo "  ./target-win/debug/obscene-tool.exe hw launch OBSC00002"
echo
echo "If install fetches only one range and no body, check 'hw sh ps' for a STOPped eboot.bin"
echo "holding the title id, and rebuild under a different CONTENT_ID. (D223)"
