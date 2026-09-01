#!/bin/sh
# Lists what a build imports, and which of those the manifest has no library for.
#
# The gap is the point: a symbol the loader must resolve and the manifest cannot place
# would be imported from library zero, which resolves to nothing at all. Better to see
# the list than to ship a module that half-resolves.
set -e
BUILD="${BUILD:-/tmp/obs}"
TOOL="${TOOL:-/tmp/obscene-tool-target/release/obscene-tool}"

# `imports` indents its list for reading; strip that before comparing.
# `imports` prints a header and indents its list for reading. Keep only lines that
# are a single C identifier, which is what a symbol name is and what the headers are
# not.
"$TOOL" imports "$BUILD/obscene.elf"     | sed 's/^ *//'     | grep -E '^[A-Za-z_][A-Za-z0-9_]*$'     | sort -u > "$BUILD/want.txt"
cut -d' ' -f2 "$BUILD/symbols.txt" | sort -u > "$BUILD/known.txt"

printf 'imported: %s\n' "$(wc -l < "$BUILD/want.txt")"
printf 'manifest: %s\n' "$(wc -l < "$BUILD/known.txt")"
printf '\nimported, with no library in the manifest:\n'
comm -23 "$BUILD/want.txt" "$BUILD/known.txt" | sed 's/^/  /'
printf '\nin the manifest but not imported (harmless, but worth seeing):\n'
comm -13 "$BUILD/want.txt" "$BUILD/known.txt" | sed 's/^/  /' | head -20
