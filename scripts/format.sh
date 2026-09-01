#!/bin/sh
# Formats in place without renaming.
#
# `clang-format -i` writes by rename, which a mounted Windows share refuses after the
# temp file is written - it leaves `foo.c.temp-stream-XXXX` and no `foo.c`. Redirect
# and truncate instead: same result, no rename. See docs/WORKLOG.md.
set -e
find src include -name '*.c' -o -name '*.h' | while read -r f; do
  clang-format "$f" > /tmp/obs-fmt.out && cat /tmp/obs-fmt.out > "$f"
done
echo "formatted"
