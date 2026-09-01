#!/bin/bash
# Build obscene.pkg and put it on the console. One command, both halves.
#
#     wsl.exe -d Ubuntu -- bash <OOPS>/obscene/scripts/oops-rebuild-pkg.sh
#
# or from a Windows shell - the script re-enters WSL itself rather than failing:
#
#     bash <OOPS>/obscene/scripts/oops-rebuild-pkg.sh
#
# Three phases, and the machine each runs on is forced, not chosen:
#
#   1. selfish     WSL      the formats, built first...
#   2. make pkg    WSL      ...so `make pkg` links the sources as they are now
#   3. hw install  Windows  the console fetches the package from us, over HTTP
#
# Phase 1 before 2 is the whole reason this script existed to begin with: `make pkg` calls the
# `selfish` binary, so editing selfish and rebuilding only obSCEne produces a package from the
# *previous* selfish - a difference that does not surface until a console refuses the result.
#
# Phase 3 is the addition. Why it cannot run where 1 and 2 do:
#
#   * `make pkg` needs `module`, which needs `$(BUILD)/symbols.txt`, produced by *running*
#     `obscene-host` - a POSIX binary (`unistd.h`, BSD sockets, pthreads). `BUILD` must be
#     Linux-local as well: a Windows mount cannot carry the execute bit. (D012)
#   * `hw install` is the opposite direction. It serves the package and has the console connect
#     *in* to fetch it (`pkg_install` takes a URL; a bare path and `file://` are both refused).
#     `pros_core::handover` binds the interface that routes to the target - under WSL2's default
#     NAT that is `172.24.x.x`, which the console cannot reach. It then fails in the most
#     misleading way available: the shell prints its usual line, the console never asks, and
#     `fetched 0 time(s)` is the only tell. From Windows the same code binds `192.168.1.x` and
#     the console comes. (CLAUDE.md, "Which half runs where")
#
# So the split is crossed here, with WSL interop, instead of being left as two commands in two
# shells that somebody has to remember the order of.
#
#   --build-only     phases 1-2; stop with the package built
#   --deploy-only    phase 3 only, against the package already built
#   --launch         after a successful install, `hw launch` the title (opt-in, never default)
#   --seconds N      how long to wait on the installer          (default 180)
#   --name NAME      which registered console, if several
#   --gen N          console generation to build for            (default 4)
#   --no-cache       build without the sccache compiler/crate cache
#   --jobs N         parallel C compilation width               (default: all cores)
# Paths are derived from this script's own location rather than hardcoded, so the
# collection works wherever it is cloned. `$OOPS` is the parent holding all four projects.
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
OOPS="$(cd "$REPO/.." && pwd)"

set -e

# Run everything under WSL, whichever shell this was started from.
#
# **Before parsing flags, not after.** The arg loop below consumes `$@`, so a re-entry placed
# after it would `exec` into WSL with no arguments at all - which silently dropped every flag
# and left `--gen`, `--build-only` and the rest working only when they happened to match a
# default. The fix is to re-enter first and let the WSL side do all the parsing on the original
# arguments; only the caller's *environment* variables (which an `exec` does not carry) are
# forwarded explicitly.
#
# The re-entry is guarded by an environment variable rather than by testing twice, so a distro
# that somehow reports itself wrongly loops once and stops rather than forever.
if ! grep -qi microsoft /proc/version 2>/dev/null; then
    [ -n "$OBS_PKG_REENTERED" ] && { echo "oops-rebuild-pkg.sh: re-entered WSL and still not in WSL" >&2; exit 1; }
    here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    # /c/... (Git Bash) -> /mnt/c/... (WSL). Anything already under /mnt is left alone.
    linux="$(printf '%s' "$here/oops-rebuild-pkg.sh" | sed 's|^/\([a-zA-Z]\)/|/mnt/\1/|')"
    echo "oops-rebuild-pkg.sh: not in WSL - re-entering ${WSL_DISTRO:-Ubuntu}"
    exec wsl.exe -d "${WSL_DISTRO:-Ubuntu}" -- \
        env OBS_PKG_REENTERED=1 PROC_SDK="${PROC_SDK:-}" BUILD_ID="${BUILD_ID:-}" DISPLAY_PAIR="${DISPLAY_PAIR:-}" DISPLAY_MEM="${DISPLAY_MEM:-}" DISPLAY_PROBE="${DISPLAY_PROBE:-}" CACHE="${CACHE:-}" JOBS="${JOBS:-}" bash "$linux" "$@"
fi

TITLE_ID="OBSC00001"

build=1
deploy=1
launch=0
seconds=180
console_name=""
GEN="${GEN:-4}"

while [ $# -gt 0 ]; do
    case "$1" in
        --build-only)  deploy=0 ;;
        --deploy-only) build=0 ;;
        --launch)      launch=1 ;;
        --seconds)     seconds="$2"; shift ;;
        --name)        console_name="$2"; shift ;;
        --gen)         GEN="$2"; shift ;;
        --no-cache)    CACHE=0 ;;
        --jobs)        JOBS="$2"; shift ;;
        -h|--help)     sed -n '2,45p' "$0"; exit 0 ;;
        *) echo "oops-rebuild-pkg.sh: unknown option $1 (try --help)" >&2; exit 2 ;;
    esac
    shift
done

export PATH="$HOME/.cargo/bin:$PATH"

# Compiler and crate caching, on by default when it is installed, off with --no-cache.
#
# One tool for both languages: sccache caches the Rust crates (selfish, the tool) through
# RUSTC_WRAPPER and the C objects (the eboot, the module, the host) by wrapping the compiler.
# The heavy one is `surface.c`, which expands a thirty-five-thousand-symbol census on every
# build - a cache hit turns that from seconds into nothing. It only helps a rebuild, never
# hurts one, so it is the default; `--no-cache` is here for the rare case of ruling it out.
#
# `CARGO_INCREMENTAL=0` because sccache and cargo's own incremental compilation do not combine
# - sccache declines to cache an incremental build - and this script does whole rebuilds, which
# is exactly the shape the cache is for. An interactive `cargo build` in a checkout is left
# alone; this only sets it for the builds this script runs.
CACHE="${CACHE:-1}"
# The C compiler `make` is given: bare `clang` by default (matching the Makefile), wrapped with
# the cache when there is one, so the only thing that changes is the wrapper.
CC_FOR_MAKE="clang"
if [ "$CACHE" = 1 ] && command -v sccache >/dev/null 2>&1; then
    export RUSTC_WRAPPER=sccache
    export CARGO_INCREMENTAL=0
    CC_FOR_MAKE="sccache clang"
    echo "cache: sccache $(sccache --version 2>/dev/null | awk '{print $2}')"
elif [ "$CACHE" = 1 ]; then
    echo "cache: off (sccache not installed - see scripts/README.md)"
else
    echo "cache: off (--no-cache)"
fi

# Every core the box has, on the C build. `make` orders the targets by their prerequisites, so
# the parallelism is safe; the generated headers are committed, not raced into existence here.
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 4)}"

OOPS="${OOPS:-$OOPS}"
SELFISH="${SELFISH:-$OOPS/selfish}"
OBSCENE="${OBSCENE:-$OOPS/obscene}"
BUILD="${BUILD:-$HOME/obs-pkg}"

# Where the Windows half reads from. The package is built Linux-local (see above) and a Windows
# process cannot open `$HOME` inside the distro without going through `\wsl$`, so it is copied
# into the repository - `*.pkg` is gitignored, so this is a working file, not a commit.
staged="$OBSCENE/obscene.pkg"

if [ "$build" = 1 ]; then
    # Nothing from a previous run survives into this one.
    #
    # This was a real bug in the first version: `make pkg` failed, the `| grep` pipeline swallowed
    # its exit status, the `[ -f ... ]` guard found *yesterday's* package still sitting there, and
    # the script staged and shipped it while printing "built:". A stale package installed under
    # the belief it is the current one is the worst outcome this script can produce, so the file
    # is removed first and both build steps are checked through `PIPESTATUS` rather than by
    # looking for output that a previous run could have left behind.
    rm -f "$BUILD/obscene.pkg" "$staged"

    echo "=== 1/3  selfish (the formats) ==="
    CARGO_TARGET_DIR="$HOME/selfish-target" \
        cargo build --manifest-path "$SELFISH/Cargo.toml" -p selfish-pkg -p selfish-cli 2>&1 \
        | grep -E "^error|Finished" || true
    [ "${PIPESTATUS[0]}" = 0 ] || { echo "oops-rebuild-pkg.sh: selfish did not build - stopping before make pkg, because a package built against the previous selfish is not the one you edited" >&2; exit 1; }

    # Everything a sweep proved ends the process, applied to the package build.
    #
    # `make pkg` has no exclusions of its own, and the sweep's working list lives in a
    # different build directory. Without this, the package that goes to the console is the one
    # with all ten known process kills back in it - and the failure mode is a console crash
    # and a report truncated at whichever one it hit first, which reads like a new finding
    # rather than a build mistake.
    #
    # EXCLUDE already in the environment wins, so re-testing the finding on new firmware is
    # `EXCLUDE= bash scripts/oops-rebuild-pkg.sh` and needs no edit here.
    record="$OBSCENE/data/hardware/crashers.txt"
    if [ -z "${EXCLUDE+set}" ] && [ -f "$record" ]; then
        EXCLUDE=$(sed -e 's/#.*//' -e '/^[[:space:]]*$/d' "$record" | tr '\n' ' ')
        echo "excluding $(sed -e 's/#.*//' -e '/^[[:space:]]*$/d' "$record" | wc -l) known process kill(s) from $record"
    fi
    echo "=== 2/3  obscene.pkg ==="
    cd "$OBSCENE"
    CARGO_TARGET_DIR="$HOME/obs-tool-target" \
        make -j"$JOBS" pkg CC="$CC_FOR_MAKE" GEN="$GEN" HARDWARE=1 BUILD="$BUILD" SELFISH="$SELFISH" EXCLUDE="$EXCLUDE" PROC_SDK="${PROC_SDK:-0}" BUILD_ID="${BUILD_ID:-dev}" DISPLAY_PAIR="${DISPLAY_PAIR:-0}" DISPLAY_MEM="${DISPLAY_MEM:-3}" DISPLAY_PROBE="${DISPLAY_PROBE:-0}" 2>&1 \
        | grep -vE "^clang|^\s+-" || true
    [ "${PIPESTATUS[0]}" = 0 ] || { echo "oops-rebuild-pkg.sh: make pkg failed (the compiler output is above)" >&2; exit 1; }

    [ -f "$BUILD/obscene.pkg" ] || { echo "oops-rebuild-pkg.sh: make pkg reported success and produced no package" >&2; exit 1; }
    cp "$BUILD/obscene.pkg" "$staged"
    echo "built:  $staged  ($(stat -c %s "$staged") bytes)"
fi

if [ "$deploy" = 0 ]; then
    echo "stopped before the install (--build-only). To send it later:"
    echo "    bash scripts/oops-rebuild-pkg.sh --deploy-only"
    exit 0
fi

[ -f "$staged" ] || { echo "oops-rebuild-pkg.sh: no package at $staged - run without --deploy-only" >&2; exit 1; }

echo
echo "=== 3/3  install (Windows, so the console can reach us) ==="

# The Windows-native tool. Built here rather than assumed present, for the same reason phase 1
# precedes phase 2: an install run through a stale binary is an install of somebody else's
# handover code, and it fails in ways that read like the package's fault.
#
# `WSLENV=VAR/p` is what carries a variable across the boundary *and* translates it as a path;
# without it a Windows process started from WSL sees none of this shell's environment.
tool="$OBSCENE/tool"
target="$tool/target-win"
exe="$target/debug/obscene-tool.exe"

( cd "$tool" && CARGO_TARGET_DIR="$target" WSLENV=CARGO_TARGET_DIR/p \
    cargo.exe build --bin obscene-tool 2>&1 | tr -d '\r' | grep -E "^error|Finished" ) || true
[ -f "$exe" ] || { echo "oops-rebuild-pkg.sh: no $exe - the Windows build did not produce one" >&2; exit 1; }

# The binary is executed by its *Linux* path - WSL interop runs a `.exe` from `/mnt/...`, but
# bash cannot exec a Windows-style one. Only the arguments cross as Windows paths, because it
# Windows process that has to open them.
win_pkg="$(wslpath -w "$staged")"
name_arg=()
[ -n "$console_name" ] && name_arg=(--name "$console_name")

# Reachability before the transfer, so "the console is off" and "the console refused the
# package" are two different messages rather than one confusing one.
( cd /mnt/c && "$exe" hw check "${name_arg[@]}" 2>&1 | tr -d '\r' )

echo
# Install and launch are ONE action when launching, and two when not.
#
# The app0 mount re-fetches the image from the URL the install served, so a server that stops
# between a separate install and launch leaves a dead URL and the launch fails
# (`mountApp0Dir 0x80020002`) - the tool's own `deploy` arm documents this. `hw deploy` holds a
# single server across the install, the promote wait, and the launch. A plain `install` is right
# only when nothing is being launched.
#
# When launching, the report is captured *here*, as the run happens, rather than pulled from disk
# afterwards. It leaves the sandbox on the system log, not the file sink (D233): the file the eboot
# writes lands locked inside the title's sandbox, where ftpsrv cannot reach it, so the log is the
# one readable channel. `report` connects out and keeps only the `OBS|` records; started before the
# launch and given a window that outlasts `hw deploy`'s settle, it is listening the whole time they
# flow. This is why the old `hw pull /data/obscene-report.txt` is gone - it fetched a file the
# console will not hand over.
if [ "$launch" = 1 ]; then
    report="$OBSCENE/reports/hardware/console-klog.txt"
    mkdir -p "$(dirname "$report")"
    report_win="$(wslpath -w "$report")"
    ( cd /mnt/c && "$exe" report --seconds "$((seconds + 60))" --into "$report_win" "${name_arg[@]}" 2>&1 | tr -d '\r' ) &
    report_pid=$!
    sleep 3
    ( cd /mnt/c && "$exe" hw deploy "$win_pkg" --into "$TITLE_ID" --seconds "$seconds" "${name_arg[@]}" 2>&1 | tr -d '\r' ) \
        | tee "$BUILD/install.log" || true
    wait "$report_pid" 2>/dev/null || true
else
    ( cd /mnt/c && "$exe" hw install "$win_pkg" --seconds "$seconds" "${name_arg[@]}" 2>&1 | tr -d '\r' ) \
        | tee "$BUILD/install.log" || true
fi

echo
if grep -q "fetched 0 time(s)" "$BUILD/install.log" 2>/dev/null; then
    echo "oops-rebuild-pkg.sh: the console never asked for the file, so the package was never judged."
    echo "        Nothing in this run says anything about the package itself. Check that the"
    echo "        console's registered address is current and that the serving interface is the"
    echo "        LAN one - this is the WSL-NAT failure, seen from Windows."
    exit 1
fi

# The report is already on disk: the concurrent `report` capture above wrote it and printed its
# outcome (the record count and the meta/build/tally/end lines). `reports/` is gitignored, so it
# is a working file - copy it into `data/hardware/` if a run is worth keeping. Capturing it again
# from /data over FTP is what this script used to do and what does not work: that file is sealed
# inside the title's sandbox (D233).
