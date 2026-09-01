#!/bin/sh
# Formats in place without renaming.
#
# `clang-format -i` writes by rename, which a mounted Windows share refuses after the
# temp file is written - it leaves `foo.c.temp-stream-XXXX` and no `foo.c`. Redirect
# and truncate instead: same result, no rename. See docs/WORKLOG.md.
#
# # Both languages, because the gate checks both
#
# This formatted C and nothing else, while `lint.sh` checks `cargo fmt --check` and, on a
# failure, prints "tool formatting is off - run ./bin/obscene fmt". That advice could not
# work: the command it names never touched a `.rs` file. A gate that reports a fault and
# names a fix that does not fix it is worse than one that says nothing.
set -e

if command -v clang-format >/dev/null 2>&1; then
    find src include -name '*.c' -o -name '*.h' | while read -r f; do
      clang-format "$f" > /tmp/obs-fmt.out && cat /tmp/obs-fmt.out > "$f"
    done
    echo "formatted C"
else
    # Not fatal: the Rust half is still worth doing, and CI checks the C separately.
    echo "clang-format not found - skipped the C" >&2
fi

if [ -f "$HOME/.cargo/env" ]; then
    . "$HOME/.cargo/env"
fi
( cd tool && cargo fmt --all )
echo "formatted the tool"
