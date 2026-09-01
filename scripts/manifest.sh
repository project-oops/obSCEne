#!/bin/sh
# Emits the symbol -> library manifest the module build needs.
#
# Comes from the host build rather than a committed data file: the check tables are
# the only place the association exists, and a second copy would go stale silently.
# A symbol that gains a library in the tables and not here would be imported from the
# wrong one, which resolves to nothing rather than failing loudly.
set -e
BUILD="${BUILD:-/tmp/obs}"
make host BUILD="$BUILD" >/dev/null
"$BUILD/obscene-host" --symbols > "$BUILD/symbols.txt"
printf 'symbols: %s\n' "$(wc -l < "$BUILD/symbols.txt")"
printf 'libraries:\n'
cut -d' ' -f1 "$BUILD/symbols.txt" | sort | uniq -c | sort -rn
