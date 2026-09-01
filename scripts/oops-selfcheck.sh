#!/bin/bash
# Everything that has to pass in both repositories, in one command.
#
#     wsl.exe -d Ubuntu -- bash <OOPS>/obscene/scripts/oops-selfcheck.sh
#
# Prints nothing when clean. selfish is checked first because obSCEne takes it as a path
# dependency, so a break there shows up here as a confusing failure in a consumer.
# Paths are derived from this script's own location rather than hardcoded, so the
# collection works wherever it is cloned. `$OOPS` is the parent holding all four projects.
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
OOPS="$(cd "$REPO/.." && pwd)"

set -u
export PATH="$HOME/.cargo/bin:$PATH"
fail=0
cd "$OOPS/selfish"
CARGO_TARGET_DIR="$HOME/selfish-target" cargo test -q --workspace 2>&1 \
    | grep -E "^error|result: FAILED|panicked" && fail=1
CARGO_TARGET_DIR="$HOME/selfish-target" cargo clippy -q --workspace --all-targets 2>&1 \
    | grep -E "^error" && fail=1
cd "$OOPS/obscene"
CARGO_TARGET_DIR="$HOME/obs-tool-target" cargo test -q --manifest-path tool/Cargo.toml 2>&1 \
    | grep -E "^error|result: FAILED|panicked" && fail=1
CARGO_TARGET_DIR="$HOME/obs-tool-target" cargo clippy -q --manifest-path tool/Cargo.toml --all-targets 2>&1 \
    | grep -E "^error" && fail=1
exit $fail
