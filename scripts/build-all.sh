#!/bin/sh
# Builds every target and re-derives the tag assignment from the module.
#
# The three builds share every check; what differs is the loader each is shaped for.
# Building one and not the others is how a change that suits a module and breaks a
# payload gets in.
set -e
BUILD="${BUILD:-/tmp/obs}"
TOOL="${TOOL:-/tmp/obscene-tool-target/release/obscene-tool}"

for target in host payload module; do
    printf '=== %s\n' "$target"
    make "$target" BUILD="$BUILD"
done

printf '=== derive (module)\n'
"$TOOL" derive "$BUILD/obscene.module.elf"

# A payload is a plain ELF with no vendor segment, so there is nothing to derive from.
# It must say that rather than pass: a check that quietly succeeds on a file it cannot
# read is worse than no check.
printf '=== derive (payload, must refuse)\n'
pelf="$BUILD/obscene-payload.elf"
[ -f "$pelf" ] || pelf="$BUILD/obscene.elf"
if "$TOOL" derive "$pelf"; then
    echo "FAIL: derive accepted a payload" >&2
    exit 1
fi
echo "refused, as it should"
