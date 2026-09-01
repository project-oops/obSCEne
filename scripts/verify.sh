#!/bin/sh
# Everything that has to pass before a change is done.
#
# One script rather than a remembered sequence: the host build, the module build and
# the tool tests each catch a different class of mistake, and the one skipped is
# reliably the one that would have caught it.
#
# # This script used to always exit 0
#
# Every stage hid its own failure, and each looked reasonable on its own:
#
#   (cd tool && cargo test --quiet 2>&1 | tail -2)   status is tail's, not cargo's
#   sh scripts/build-all.sh ... || true               a compile error, swallowed twice
#   make check ... && echo ok || echo FAILED          prints FAILED, returns 0
#   sh scripts/sweep-build.sh 2>&1 | head -1          status is head's
#
# So "everything that has to pass" could not fail, and a run that printed FAILED in the
# middle still exited 0. A whole review's worth of defects survived behind it - and the
# only reason any were caught by hand is that somebody was reading the output rather
# than trusting the exit code, which is the opposite of what a gate is for.
#
# The rule now: **every stage's status is checked, and output filtering never sits
# between a command and its exit code.** Where a pipeline is wanted for readability, the
# command runs first into a file and the file is filtered afterwards.
set -e
BUILD="${BUILD:-/tmp/obs}"
export CARGO_TARGET_DIR="${CARGO_TARGET_DIR:-/tmp/obscene-tool-target}"
# Guarded by a file test, and `|| true` is not enough on its own.
#
# `.` is a POSIX *special built-in*, and a special built-in that fails makes a POSIX shell
# exit immediately - before `||` is ever consulted. So when this file is absent, which is
# the normal state of a Windows rustup install, the whole gate exited at line 28 with **no
# output and status 1**. It looked exactly like a failing verification and was a missing
# convenience script; cargo was on PATH the entire time.
#
# That is the same defect this file was rewritten to remove, arriving from the other
# direction: the first version could never fail, and this could fail without saying anything.
# A gate has to be honest in both directions.
if [ -f "$HOME/.cargo/env" ]; then
    . "$HOME/.cargo/env"
fi

log="${TMPDIR:-/tmp}/obscene-verify.$$"
trap 'rm -f "$log"' EXIT

failed=0
# Records a failure and keeps going, so one run reports every broken stage rather than
# only the first. A gate that stops at the first fault turns a five-minute fix into five
# separate five-minute rounds.
note_failure() {
    printf '    FAILED: %s\n' "$1"
    failed=1
}

printf '=== tool tests\n'
if (cd tool && cargo test --quiet >"$log" 2>&1); then
    tail -2 "$log"
else
    tail -20 "$log"
    note_failure "cargo test"
fi

printf '=== tool lints\n'
if sh scripts/lint.sh; then
    :
else
    note_failure "clippy"
fi

# CI runs this and nothing local did, so the tree drifted out of format and stayed there.
# A gate that is weaker than CI produces exactly this: a change that passes locally and
# fails on push, which trains people to distrust the local gate.
printf '=== tool formatting\n'
if (cd tool && cargo fmt --check >"$log" 2>&1); then
    printf 'formatted\n'
else
    head -20 "$log"
    note_failure "cargo fmt --check"
fi

# The rule in CLAUDE.md that nothing enforced. Thirty-eight checks violated it, and the
# violation makes the `try` record name a function that was never reached - so this gates
# the program's central invariant, not a style preference.
printf '=== cross-symbol guards\n'
# Rust, not Python, and the port paid for itself before it was finished.
#
# The Python version required the symbol column to be an identifier, so it silently skipped
# eight real rows - every check whose symbol is descriptive rather than a name, including
# `910-bulk/probe`, which calls arbitrary censused symbols and is the check a guard rule
# least wants to miss. It also assumed every runner is called `check_*`; the blind prober's
# is `run_bulk`. Neither fault could show up as a failure, only as a smaller number nobody
# was comparing against anything.
if (cd tool && cargo run --quiet -- guards --root ..) >"$log" 2>&1; then
    head -1 "$log"
else
    cat "$log"
    note_failure "obscene-tool guards"
fi

# Capability ordering. A check that requires something granted later never runs at all, and
# says so in words indistinguishable from a platform that genuinely lacks it: "a prerequisite
# capability was not established". Two checks in `018-relational` required `OBS_CAP_FILE`,
# which `040-file` grants twenty-two sections later, and neither had ever run on any target
# from the day they were written. The reports were read many times and looked reasonable.
printf '=== capability ordering\n'
if (cd tool && cargo run --quiet -- caps --root ..) >"$log" 2>&1; then
    head -1 "$log"
else
    cat "$log"
    note_failure "obscene-tool caps"
fi

# Drift. Both exist because prose that quotes the code has no mechanism to stay true: the
# README said 79 checks when there were 106, and `make target` was named in four files
# without ever having been a rule.
printf '=== documentation counts\n'
if (cd tool && cargo run --quiet -- counts --root .. --check) >"$log" 2>&1; then
    head -1 "$log"
else
    cat "$log"
    note_failure "obscene-tool counts --check"
fi

# The per-loader results table. Checked only when the reports it was built from are still
# present: it is generated from run output, and a fresh clone has none. Kyty and orbistoun
# are included deliberately despite producing no records - "produced nothing and did not reach
# the end" is a fact about a loader and belongs in a compatibility table, not only in prose.
# They are silent for unrelated reasons (D080; a bare-ELF container path), which is itself the
# argument for listing both rather than one line saying "some loaders do not report".
# The column list lives here and in docs/COMPATIBILITY.md's "Regenerating" block, and the
# two have to agree or this gate reports drift that is really a missing argument. Adding a
# loader means editing both - which is the cost of the table being generated rather than
# written, and cheaper than the table being wrong.
if [ -f reports/host.txt ] && [ -f reports/shadps4.txt ] \
    && [ -f reports/kyty.txt ] && [ -f reports/fpps4.txt ] \
    && [ -f reports/ps5pcem.txt ] && [ -f reports/orbistoun.txt ]; then
    printf '=== compatibility table\n'
    if (cd tool && cargo run --quiet -- compat --into ../docs/COMPATIBILITY.md --check \
        "host=../reports/host.txt" "shadPS4=../reports/shadps4.txt" \
        "PS5PCEM=../reports/ps5pcem.txt" "fpPS4=../reports/fpps4.txt" \
        "kyty=../reports/kyty.txt" "orbistoun=../reports/orbistoun.txt") >"$log" 2>&1; then
        head -1 "$log"
    else
        cat "$log"
        note_failure "obscene-tool compat --check"
    fi
fi

# The command protocol, and the checker that guards it.
#
# Two stages rather than one, and the order matters: the self-test proves the checker can
# say no before the checker is believed when it says yes. This project has shipped two
# gates that could never fail, and a third guarding a contract two implementations are
# built against is not the place to find out about a fourth.
printf '=== protocol checker self-test\n'
if (cd tool && cargo run --quiet -- protocol --root .. --selftest) >"$log" 2>&1; then
    tail -1 "$log"
else
    cat "$log"
    note_failure "obscene-tool protocol --selftest"
fi

printf '=== captured protocol exchanges\n'
if (cd tool && cargo run --quiet -- protocol --root ..) >"$log" 2>&1; then
    head -1 "$log"
else
    cat "$log"
    note_failure "obscene-tool protocol"
fi

# The GPU capability, gated conditionally so a machine without the GPU toolchain still
# passes the rest - the same way the compatibility table only runs when its reports exist.
# A capability outside the gate rots (the project's own rule), and the GPU code compiles
# under the strict warnings only if something keeps checking it.
if command -v glslangValidator >/dev/null 2>&1; then
    printf '=== embedded shaders match their source\n'
    if (cd tool && cargo run --quiet -- shaders --root .. --check) >"$log" 2>&1; then
        head -1 "$log"
    else
        cat "$log"
        note_failure "obscene-tool shaders --check"
    fi
fi

# The GPU ISA surface census. Unconditional - unlike the shader check it needs no toolchain,
# because its source of truth is the in-repo classification table; the LLVM .td, when the VM
# has it, only adds a cross-check that no named intrinsic has vanished and no new scalar-math
# one has appeared unclassified. A census that is not gated drifts from the hardware it maps.
printf '=== GPU surface census\n'
if (cd tool && cargo run --quiet -- gpusurface --root .. --check) >"$log" 2>&1; then
    head -1 "$log"
else
    cat "$log"
    note_failure "obscene-tool gpusurface --check"
fi

# A GPU=1 compile, when Vulkan headers are present. Compile only, not a run: it proves the
# backend and section still build under -Werror -Wconversion, without depending on a working
# software rasteriser or on timing. The dispatch itself is exercised by hand against llvmpipe
# and, in time, on the Deck.
if [ -f /usr/include/vulkan/vulkan.h ]; then
    printf '=== GPU backend builds (GPU=1)\n'
    if make host GPU=1 BUILD="$BUILD-gpu" >"$log" 2>&1; then
        printf 'gpu build: ok\n'
    else
        tail -20 "$log"
        note_failure "make host GPU=1"
    fi

    # The golden regression check, which does run the GPU - but only ever asserts against a
    # matching device and skips otherwise, so it is not the fragile "needs a rasteriser"
    # dependency the compile-only rule above guards against. On the build VM's llvmpipe it
    # catches any change to a kernel's output the reference cannot (the transcendentals); on a
    # different device, or none, it prints why it skipped and passes. See scripts/gpu-golden.sh.
    printf '=== GPU golden regression\n'
    if sh scripts/gpu-golden.sh --check >"$log" 2>&1; then
        tail -1 "$log"
    else
        cat "$log"
        note_failure "scripts/gpu-golden.sh --check"
    fi
fi

# The generated censuses.
#
# `corpus.h` and `nids.h` are generated from data/ and committed, so the data can move
# while the headers stay behind - the same drift `obscene-tool counts` gates one level up, and
# ungated until now. A stale census reports an absence for a symbol the corpus no longer
# claims, which reads as a platform gap rather than as a stale file.
#
# The arguments must match how they were generated, so they live here beside the check
# that enforces them.
printf '=== generated censuses
'
# The curated census too, which was never gated and had quietly stopped regenerating: ten
# names promoted to `platform.h` were removed from the header and from the refusal list, and
# left in the generator's own group lists. Nothing noticed, because nothing ran it.
if (cd tool && cargo run --quiet -- surface --root .. --check) >"$log" 2>&1; then
    tail -1 "$log"
else
    cat "$log"
    note_failure "obscene-tool surface --check"
fi

# The SELF header table the on-console audit (048-selfaudit) compares against. Its source is
# selfish's data/self-format.tsv; the generated C is only trustworthy if a change to that table
# without regenerating this fails here. Same drift discipline as every other generated header.
printf '=== SELF header table\n'
if (cd tool && cargo run --quiet -- selfheader --root .. --check) >"$log" 2>&1; then
    tail -1 "$log"
else
    cat "$log"
    note_failure "obscene-tool selfheader --check"
fi

for which in corpus nids; do
    if (cd tool && cargo run --quiet -- census "$which" --root .. --platform --check)         >"$log" 2>&1; then
        tail -1 "$log"
    else
        cat "$log"
        note_failure "obscene-tool census $which --check"
    fi
done

# The other half of the census drift, and the half that was never gated.
#
# The check above catches the generated headers falling behind `data/`. This catches
# `data/` falling behind the emulators it was mined out of - an emulator gains names in a
# release, the corpus does not, and obSCEne reports absences for a surface it never asked
# about while the census reads as complete.
#
# It compares recorded commits, not contents: re-mining costs minutes and needs both the
# checkouts and the firmware tree, so it says "this corpus has not been shown the current
# commit" rather than "these names changed". On a machine without the checkouts it says so
# and passes, the same way the compatibility table only runs when its reports exist.
#
# **The build VM is such a machine**, and this is worth knowing before relying on it: the
# emulator toolkit lives on the Windows host and is not mounted, so a normal
# `multipass exec ... verify.sh` prints "sources not present, not checked" and moves on.
# The check only bites where mining happens, which is the host. That is the right place -
# nobody re-mines in the VM - but a gate that skips in the place the gate usually runs is
# not covering anyone, and should not be mistaken for cover.
# The decision log's own index, generated from its entries.
#
# `DECISIONS.md` is exempt from doccheck's accuracy rules because it is a dated record, but
# an index is not a record - it is a claim about the file it sits in, and it goes stale the
# moment an entry is appended. Which is every session.
printf '=== decision index\n'
if (cd tool && cargo run --quiet -- decisions --root .. --check) >"$log" 2>&1; then
    tail -1 "$log"
else
    cat "$log"
    note_failure "obscene-tool decisions --check"
fi

printf '=== corpus provenance\n'
if (cd tool && cargo run --quiet -- corpus --root ..) >"$log" 2>&1; then
    tail -1 "$log"
else
    cat "$log"
    note_failure "obscene-tool corpus"
fi

printf '=== documentation references\n'
if (cd tool && cargo run --quiet -- doccheck --root ..) >"$log" 2>&1; then
    head -1 "$log"
else
    cat "$log"
    note_failure "obscene-tool doccheck"
fi

# Prose that describes behaviour, against the behaviour. `doccheck` above catches a document
# naming something that does not exist; this catches one describing something the source no
# longer does. `PROTOCOL.md` spent part of a day stating "It binds loopback by default" after
# that had been reverted from `net_posix.c`, and nothing failed. (D170)
printf '=== anchored prose still describes the source\n'
if (cd tool && cargo run --quiet -- claims --root ..) >"$log" 2>&1; then
    tail -1 "$log"
else
    cat "$log"
    note_failure "obscene-tool claims"
fi

printf '=== all three targets, and the derivation\n'
if sh scripts/build-all.sh >"$log" 2>&1; then
    grep -E '^(===|ok |FAIL|--  |refused|the assignment)' "$log" || true
else
    tail -20 "$log"
    note_failure "build-all.sh"
fi

printf '=== harness self-check\n'
if make check BUILD="$BUILD" >"$log" 2>&1; then
    printf 'check: ok\n'
else
    tail -20 "$log"
    note_failure "make check"
fi

# The parser against reality. `guards`, `caps` and `counts` all read the check tables with the
# same regular expression, so a row that expression cannot see disappears from all three at
# once - silently, and by producing a *smaller* number, which `sections.rs` says no gate can
# catch "because a gate compares against nothing".
#
# A report is the thing to compare against: the harness walks the tables at run time and emits
# a result per check, so it says what actually ran. A comment placed inside a row's braces hid
# `015-sync/event-flag-round-trip` from every gate for part of a day and nothing failed. (D168)
printf '=== the parser sees every check that ran\n'
if "$BUILD/obscene-host" > "$BUILD/rows-report.txt" 2>/dev/null || true; then
    if (cd tool && cargo run --quiet -- rows --root .. \
        --report "$BUILD/rows-report.txt") >"$log" 2>&1; then
        tail -1 "$log"
    else
        cat "$log"
        note_failure "obscene-tool rows"
    fi
fi

# Last, and it has to be last. Both build-all.sh and `make check` build the module with
# no exclusions - correct for proving it builds, wrong for anything that then runs it.
# Leaving the sweep build in place means a verify run never hands the next step a module
# that walks straight into a known crash, which is exactly what happened twice.
printf '=== module rebuilt as the sweep list says\n'
if sh scripts/sweep-build.sh >"$log" 2>&1; then
    head -1 "$log"
else
    tail -20 "$log"
    note_failure "sweep-build.sh"
fi

if [ "$failed" -ne 0 ]; then
    printf '\nverify: FAILED\n'
    exit 1
fi
printf '\nverify: ok\n'
