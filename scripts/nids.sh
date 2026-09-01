#!/bin/sh
# Prints the encoded NID for each symbol named on the command line.
#
# Useful against an emulator log: a loader that reports "resolved <nid> as Unknown" has
# told you it does not have that function, and this is how you find out which one.
TOOL="${TOOL:-/tmp/obscene-tool-target/release/obscene-tool}"
for name in "$@"; do
    printf '%-34s %s\n' "$name" "$("$TOOL" nid "$name" | awk '/encoded/ {print $2}')"
done
