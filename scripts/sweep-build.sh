#!/bin/sh
# Builds the module excluding every check listed in the sweep file.
#
# A call that ends the process takes the rest of the suite with it, so a complete sweep
# is an iterative business: run, see which call did not return, exclude it, run again.
# The file accumulates them; this turns it into the build flag.
#
# The first run should always be made with an empty file. A crash is a finding, and the
# point of excluding one is to see what is behind it, not to stop finding them.
set -e
BUILD="${BUILD:-/tmp/obs}"
LIST="${LIST:-$BUILD/excludes.txt}"
# Seed a fresh list from the committed record of what a sweep already proved.
#
# `$BUILD/excludes.txt` is a working file in a build directory: `rm -rf $BUILD` loses it,
# and a second machine never had it. `data/hardware/crashers.txt` is the finding, and it
# cost a full sweep against a real console to obtain. Seeding from it means a rebuilt
# build directory does not silently re-run into ten known process kills.
#
# Only when the list does not exist: once a sweep is under way the file is that sweep's,
# and re-seeding it mid-run would put back an exclusion the operator deliberately cleared.
# Emptying the file to re-test the finding on new firmware stays possible - `: > $LIST`
# after this point, or SEED=0.
SEED="${SEED:-1}"
RECORD="${RECORD:-$(dirname "$0")/../data/hardware/crashers.txt}"
if [ ! -f "$LIST" ]; then
    mkdir -p "$(dirname "$LIST")"
    if [ "$SEED" = 1 ] && [ -f "$RECORD" ]; then
        grep -v "^#" "$RECORD" | grep -v "^[[:space:]]*$" > "$LIST"
        echo "seeded $(wc -l < "$LIST") exclusion(s) from $RECORD"
    else
        : > "$LIST"
    fi
fi
# Exclusions the caller wants on top of the list, without writing them into it.
#
# The list is a record of what a sweep *proved* crashes, and it persists across runs. A
# caller that wants a section left out for its own reasons - `bulk-sweep.sh` does not need
# the 35,045-symbol census re-emitted on every one of its rounds - must not be able to leave
# that behind in the file as though a crash had been found there.
EXCLUDE=$(tr '\n' ' ' < "$LIST")
EXCLUDE="$EXCLUDE $EXTRA_EXCLUDE"
printf 'excluding: %s\n' "${EXCLUDE:-(nothing)}"
# GEN says which console generation the module declares itself for, and it has to be
# passed through: shadPS4 is a previous-generation emulator and refuses a module marked
# for the current one outright, so a sweep against it needs GEN=4. Defaulting to the
# Makefile's 5 here and letting the caller override is what keeps the module honest with
# whichever loader is about to run it. See D062.
#
# BULK builds in the blind prober, which is off by default. Passed through rather than
# given a build path of its own so a bulk sweep gets the exclusion list too - 910-bulk runs
# last, so any earlier check that crashes would otherwise stop the module before it.
#
# # This line could not fail
#
# It was `make module ... 2>&1 | grep -vE ... || true`, which is the same defect verify.sh
# was rewritten for: the exit status belonged to `grep`, and `|| true` discarded even that.
# A compile error printed and the script returned success, so every caller checking this
# script's status was checking nothing and would go on to run whatever module was left in
# the build directory from last time - a stale binary reported as a fresh result.
#
# The output still gets filtered; it is just filtered after the status is taken rather than
# in front of it.
log="$BUILD/sweep-build.log"
if make module BUILD="$BUILD" EXCLUDE="$EXCLUDE" ${GEN:+GEN="$GEN"} \
    ${BULK:+BULK="$BULK"} ${BULK_START:+BULK_START="$BULK_START"} \
    ${BULK_LIMIT:+BULK_LIMIT="$BULK_LIMIT"} ${SERVE:+SERVE="$SERVE"} >"$log" 2>&1; then
    grep -vE '^clang|^    ' "$log" || true
else
    tail -20 "$log"
    echo "sweep-build: the module build failed" >&2
    exit 1
fi
