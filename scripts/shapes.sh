#!/bin/sh
# Prints the two target shapes side by side.
#
# The whole point of building both is that they are different, so it should be possible
# to see the difference at a glance rather than by squinting at readelf twice.
set -e
dir="${1:-build}"
for target in module payload; do
    file="$dir/obscene-$target.elf"
    [ -f "$file" ] || { echo "$target: not built"; continue; }
    echo "--- $target ---"
    printf '  e_type   %s\n' "$(readelf -hW "$file" | sed -n 's/^  Type: *//p')"
    printf '  osabi    %s\n' "$(readelf -hW "$file" | sed -n 's/^  OS\/ABI: *//p')"
    printf '  segments %s LOAD, vendor: %s\n' \
        "$(readelf -lW "$file" | grep -c '^  LOAD')" \
        "$(readelf -lW "$file" | grep -cE '^  (LOOS|0x61)' || echo 0)"
    printf '  imports  %s\n' "$(readelf -sW --dyn-syms "$file" 2>/dev/null | grep -c UND || echo 0)"
    printf '  a symbol %s\n' "$(readelf -sW --dyn-syms "$file" 2>/dev/null | grep UND | head -1 | sed 's/.*UND *//')"
done
