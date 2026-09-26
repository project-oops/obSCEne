#!/usr/bin/env bash
# Formats first-party C with clang-format at the pinned clang major, and the tool with rustfmt.
#
#   scripts/format.sh           format in place
#   scripts/format.sh --check   fail on any difference
#
# Generated headers are excluded. Files are rewritten by redirection, not `clang-format -i`,
# because -i renames and a Windows mount refuses the rename.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

major=$(sed -n 's/^OOPS_CLANG_MAJOR *:= *//p' toolchain.mk)
image="silkeh/clang:$major"
# Held out while another session has uncommitted work in it; the next formatting commit
# drops it.
held=(-e '^src/probe/sections/agc\.c$')
mapfile -t files < <(git ls-files '*.c' '*.h' |
    grep -v -e '^include/obscene/corpus\.h$' -e '^include/obscene/nids\.h$' \
        -e '^include/obscene/surface\.h$' -e '\.gen\.h$' -e '^src/probe/font\.c$' "${held[@]}")

if clang-format --version 2>/dev/null | grep -q "version $major\."; then
    run=()
    fmt=clang-format
elif command -v "clang-format-$major" >/dev/null 2>&1; then
    run=()
    fmt="clang-format-$major"
elif command -v docker >/dev/null 2>&1; then
    # Git Bash rewrites Unix-looking arguments; `pwd -W` gives the path the daemon mounts.
    run=(env MSYS_NO_PATHCONV=1 docker run --rm -v "$(pwd -W 2>/dev/null || pwd):/w" -w /w
        "$image")
    fmt=clang-format
else
    echo "obscene: needs clang-format $major on PATH, or docker to run $image" >&2
    exit 1
fi

if [ -f "$HOME/.cargo/env" ]; then
    # shellcheck source=/dev/null
    . "$HOME/.cargo/env"
fi

if [ "${1:-}" = "--check" ]; then
    "${run[@]}" "$fmt" --dry-run --Werror "${files[@]}"
    (cd tool && cargo fmt --check)
    echo "obscene: formatting clean (${#files[@]} C files and the tool)"
else
    # shellcheck disable=SC2016
    "${run[@]}" sh -c \
        'fmt=$1; shift; for f; do "$fmt" "$f" > /tmp/fmt.out && cat /tmp/fmt.out > "$f"; done' \
        sh "$fmt" "${files[@]}"
    (cd tool && cargo fmt)
    echo "obscene: formatted ${#files[@]} C files and the tool"
fi
