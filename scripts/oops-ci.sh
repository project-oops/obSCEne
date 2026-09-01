#!/bin/bash
# Everything CI runs, in one command.
# Paths are derived from this script's own location rather than hardcoded, so the
# collection works wherever it is cloned. `$OOPS` is the parent holding all four projects.
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
OOPS="$(cd "$REPO/.." && pwd)"

export PATH="$HOME/.cargo/bin:$PATH" CARGO_TARGET_DIR="$HOME/selfish-target"
cd "$OOPS/selfish"
cargo fmt
echo "fmt:    $(cargo fmt --check 2>&1 | grep -c 'Diff in') diffs"
echo "clippy: $(cargo clippy --all-targets --all-features -- -D warnings 2>&1 | grep -cE '^error') errors"
echo "tests:  $(cargo test -q --all-features 2>&1 | grep -cE 'FAILED|panicked') failures"
