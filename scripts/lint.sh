#!/usr/bin/env bash
# The static gate: formatting in check mode, then clippy over the tool with warnings as errors.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
export CARGO_TARGET_DIR="${CARGO_TARGET_DIR:-/tmp/obscene-tool-target}"
if [ -f "$HOME/.cargo/env" ]; then
    # shellcheck source=/dev/null
    . "$HOME/.cargo/env"
fi

bash scripts/format.sh --check
(cd tool && cargo clippy --quiet --all-targets -- -D warnings)
echo "obscene: clippy clean"
