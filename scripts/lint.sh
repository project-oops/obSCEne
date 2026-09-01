#!/bin/sh
# The tool's lints, in full.
#
# A script rather than a one-liner because the pattern needs quoting that does not
# survive being passed through two shells, and a lint run that silently matched nothing
# reads as a clean tree.
#
# # It used to print "clean" whatever happened
#
# The whole of it was:
#
#   cargo clippy --quiet --all-targets 2>&1 | grep -E '^(warning|error)' -A6 || echo "clean"
#
# Two faults in one line. Clippy's exit status was discarded by the pipe, so lints it
# reported as errors did not fail the script. And if clippy failed to *run* at all -
# wrong toolchain, no network, a broken Cargo.toml - grep matched nothing, the `||` fired,
# and it printed `clean`. A tree nobody had linted was indistinguishable from a clean one,
# which is precisely the outcome the file's own comment above says to avoid.
set -e
export CARGO_TARGET_DIR="${CARGO_TARGET_DIR:-/tmp/obscene-tool-target}"
# Guarded by a file test, because `|| true` does not help here.
#
# `.` is a POSIX *special built-in*, and a special built-in that fails makes the shell exit
# immediately - before `||` is consulted and before the redirection matters. On a machine
# without this file, which is the normal state of a Windows rustup install, this script
# exited at line 21 with **no output and status 1**: indistinguishable from a tree full of
# lints, and cargo was on PATH the whole time.
#
# `verify.sh` carries the same fix and a longer note about it. This file had the identical
# bug and was found by running it on the host rather than in the build VM.
if [ -f "$HOME/.cargo/env" ]; then
    . "$HOME/.cargo/env"
fi
cd tool

log="${TMPDIR:-/tmp}/obscene-lint.$$"
trap 'rm -f "$log"' EXIT

# `-D warnings` so a warning is a failure. This project treats a lint as a defect, and
# the crate already carries deny attributes; making it explicit here means the script
# says the same thing as the source.
if cargo clippy --quiet --all-targets --message-format short -- -D warnings >"$log" 2>&1; then
    # Formatting too, so `lint` is the whole static gate the CI job was: clippy then fmt.
    if ! cargo fmt --check; then
        printf 'tool formatting is off - run ./bin/obscene fmt
'
        exit 1
    fi
    printf 'clean\n'
    exit 0
fi

# Clippy failing to run and clippy finding lints are different problems and want
# different responses, so they are reported differently rather than both as "not clean".
#
# The match has to allow for `--message-format short`, which is what this script asks for:
# a diagnostic then reads `src/main.rs:12:5: error: ...` and does **not** begin with the
# word. Anchoring on `^(warning|error)` matched only cargo's closing summary, so CI said
# "could not compile ... due to 7 previous errors" and named none of the seven. That is the
# same blindness the header above describes, moved from the detection path into the
# reporting path.
if grep -qE '^(warning|error)|: (warning|error)(\[|:)' "$log"; then
    grep -E '^(warning|error)|: (warning|error)(\[|:)' -A6 "$log"
else
    printf 'clippy did not run:\n'
    tail -20 "$log"
fi
exit 1
