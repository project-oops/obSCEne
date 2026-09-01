#!/bin/sh
# Rebuild the tool and the module, and report the vendor segment.
#
# The loop this project runs on: change something, rebuild, hand it to an emulator,
# read the log. Scripted because the tool has to be rebuilt inside the VM too and
# forgetting that means testing the previous change again and drawing a conclusion
# from it.
set -e
BUILD="${BUILD:-/tmp/obs}"
export CARGO_TARGET_DIR="${CARGO_TARGET_DIR:-/tmp/obscene-tool-target}"
. "$HOME/.cargo/env" 2>/dev/null || true

(cd tool && cargo build --release --quiet)
make module BUILD="$BUILD" 2>&1 | grep -v '^clang' | grep -v '^    -'
