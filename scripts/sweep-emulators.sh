#!/bin/sh
# Run every loader on the same module, photograph what it drew, keep its log.
#
# `run-emulator.sh` grew a screenshot mode, a sweep mode, exclusion handling and a crash
# classifier, and the thing anybody actually wants from it - *what did each loader do* - got
# harder to reach rather than easier. This does one job in one pass and nothing else:
#
#     launch  ->  wait for the report to end  ->  photograph the window  ->  keep the log
#
# Same module everywhere, no per-loader accommodation beyond how each one is spelled on a
# command line. Every difference in the output is then a difference in the loader.
#
# Usage:
#   sh scripts/sweep-emulators.sh                     # all of them
#   sh scripts/sweep-emulators.sh shad cem            # named ones only
#   sh scripts/sweep-emulators.sh --out /tmp/x --timeout 90
# Paths are derived from this script's own location rather than hardcoded, so the
# collection works wherever it is cloned. `$OOPS` is the parent holding all four projects.
EMULATORS="${EMULATORS:-$(cd "$OOPS/.." && pwd)/emulators}"

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
OOPS="$(cd "$REPO/.." && pwd)"
set -e

OUT="${OUT:-reports/sweep}"
TIMEOUT="${TIMEOUT:-60}"
# Where the modules are built. WSL, not a VM - see CLAUDE.md.
#
# `WSL_BUILD` is a Linux-local path on purpose: a Windows mount cannot carry the execute bit,
# and `symbols.txt` is produced by *running* the host binary, so a build under /mnt/c fails.
# (D012)
WSL_DISTRO="${WSL_DISTRO:-Ubuntu}"
WSL_BUILD="${WSL_BUILD:-\$HOME/obs}"
WANTED=""

while [ $# -gt 0 ]; do
    case "$1" in
        --out) OUT="$2"; shift 2 ;;
        --timeout) TIMEOUT="$2"; shift 2 ;;
        -h|--help) sed -n '2,20p' "$0"; exit 0 ;;
        -*) echo "unknown option: $1" >&2; exit 2 ;;
        *) WANTED="$WANTED $1"; shift ;;
    esac
done

# Source tree to inspect for local behaviour changes, per loader key. A loader missing here
# is simply not checked, which is honest: absence of a label is not a claim of purity.
SOURCES="
kyty|$EMULATORS/src/Kyty/source/emulator
shad|$EMULATORS/src/shadPS4/src
cem|$EMULATORS/src/PS5PCEM/src
fp|$EMULATORS/src/fpPS4/sys
"

# Loaders, as `key|executable|launch flag|process name|generation`.
#
# The process name is not decoration and not derivable from the key: it is what Windows calls
# the running program, and it is the only handle that works for photographing or stopping one.
# The shell's `$!` is a bash process id that some loaders abandon within a second of starting -
# fpPS4 re-execs - so anything built on it silently addresses a process that no longer exists.
# (D178)
#
# Kyty is launched through `fc_script`, which takes a Lua configuration rather than a module,
# so its row carries the script name and `write_kyty_lua` below builds that configuration.
#
# It is also the one loader here whose generation is not what its name suggests. The binary is
# distributed as *KytyPS5* and reads the current-generation marker correctly, but its
# current-generation display support is three functions with no way to present a frame - where
# its previous-generation support is twelve and complete. It is a previous-generation emulator
# with a current-generation branch someone started.
#
# So it gets the previous-generation module, and that is not an accommodation: the generation
# is decided at build time, in `EI_ABIVERSION` and in the library versions the module declares,
# so handing a loader the build matching what it implements is the only way to measure it at
# all. Giving it the current-generation module measures a branch nobody finished. (D186)
LOADERS="
shad|$EMULATORS/shadps4/shadPS4.exe|-g|shadPS4|4
cem|$EMULATORS/src/PS5PCEM/zig-out/bin/game-run.exe||game-run|5
fp|$EMULATORS/src/fpPS4/fpPS4.exe|-e|fpPS4|4
kyty|$EMULATORS/src/Kyty/_Build/gcc3/fc_script.exe|@lua|fc_script|4
orbistoun|$OOPS/orbistoun/target/release/orbistoun-cli.exe|run --limit 0|orbistoun-cli|5
"

mkdir -p "$OUT"
work="${TMPDIR:-/tmp}/obscene-sweep"
rm -rf "$work"; mkdir -p "$work"

win() { if command -v cygpath >/dev/null 2>&1; then cygpath -w "$1"; else printf '%s' "$1"; fi; }

# The same path as WSL sees it.
#
# Through `cygpath -w` first, and that is not belt-and-braces. Git Bash's `/tmp` is a Windows
# temp directory under the user profile; WSL's `/tmp` is its own, inside the distro. Rewriting
# the leading `/c/` alone therefore produced a path that *looked* converted and pointed at a
# directory in the wrong filesystem, so the copy failed with "No such file or directory" while
# the source and destination both plainly existed. Ask the tool that knows.
wslpath_of() {
    if command -v cygpath >/dev/null 2>&1; then
        p=$(cygpath -m "$1")
        drive=$(printf '%s' "$p" | cut -c1 | tr 'A-Z' 'a-z')
        printf '/mnt/%s%s' "$drive" "$(printf '%s' "$p" | cut -c3-)"
    else
        printf '%s' "$1" | sed 's|^/\([a-zA-Z]\)/|/mnt/\1/|'
    fi
}

# Both generations, fetched once rather than per loader.
#
# Copied out of WSL by WSL itself, writing straight to the Windows filesystem through
# `/mnt/c`. No transfer step, no second tool, and nothing to misreport: if the file is not
# there afterwards, it was not built.
#
# This used to be `multipass transfer`, which copied the file correctly and *then* failed
# setting POSIX permissions on NTFS - reporting failure having done the job, so the exit code
# had to be ignored and the file's existence tested instead. That whole hazard is gone with the
# VM. (See CLAUDE.md on why multipass was dropped.)
for gen in 4 5; do
    src="$WSL_BUILD-f$gen/obscene.module.elf"
    # Through `bash -lc`, not bare `cp`: the build path contains `$HOME`, and a bare
    # command gets no shell to expand it - so `cp` would look for a directory literally
    # named `$HOME` and fail silently into the check below.
    # The source is double-quoted *inside* the inner shell, not single-quoted: the path
    # contains `$HOME` and single quotes would hand `cp` a directory literally named that.
    # The destination is a Windows-mount path with no expansion in it, so it stays literal.
    wsl.exe -d "$WSL_DISTRO" -- bash -lc \
        "cp \"$src\" '$(wslpath_of "$work/module-gen$gen.elf")'" 2>/dev/null || true
    if [ ! -s "$work/module-gen$gen.elf" ]; then
        echo "could not fetch $src from WSL - build it with:" >&2
        echo "  wsl.exe -d $WSL_DISTRO -- bash -lc 'export PATH=\$HOME/.cargo/bin:\$PATH; cd $REPO && make module GEN=$gen BUILD=$WSL_BUILD-f$gen'" >&2
        exit 1
    fi
done

# Stop anything of this name that is already up.
#
# A leftover instance is not a harmless duplicate: it shares the report path, so the next run's
# records interleave with its own and the extraction reads the mixture as one run. It also
# looks exactly like a hang - the stale window sits wherever it got to and never moves, because
# nothing owns it any more.
stop() {
    command -v taskkill >/dev/null 2>&1 && taskkill //F //IM "$1.exe" >/dev/null 2>&1 || true
}

# Is this loader running with local changes to its behaviour?
#
# It matters and it is not a reason to avoid patching one. A loader that aborts the moment it
# meets something it has not implemented cannot be measured at all, and patching that is the
# difference between a row of numbers and an empty row - see `patches/README.md`.
#
# What honesty costs is a **label**, not the emulator. Reverting a working patch to keep a
# table tidy throws away the measurement to protect the caption, which is backwards. So the
# sweep looks for the marker every patch in `patches/` carries and says so in the row.
patched() {
    grep -rlq 'LOCAL CHANGE (obSCEne)' "$1" 2>/dev/null && echo " [patched]" || true
}

# `grep -q` must never read a Windows program's output, and this cost four minutes a run.
#
# `grep -q` exits the moment it matches and closes the pipe. A POSIX writer gets SIGPIPE and
# dies; `tasklist.exe` is a Windows program with no such signal, so it **blocks forever**
# writing the rest of its output to a reader that is gone. The traced symptom is a loop that
# stops mid-iteration and never re-tests its own budget: Kyty ran 4.1 minutes against a
# 100-second timeout, and a 60-second run was still going at five.
#
# Same failure that hung `apt` for thirty-nine minutes today, from the other end of the pipe.
# So: `/FI` to make the output small, and `$(...)` to read it to EOF instead of abandoning it.
process_running() {
    command -v tasklist >/dev/null 2>&1 || return 1
    # MSYS_NO_PATHCONV, or Git Bash rewrites `/FI` into a Windows path and the filter is lost.
    out=$(MSYS_NO_PATHCONV=1 tasklist /FI "IMAGENAME eq $1.exe" /NH 2>/dev/null | tr 'A-Z' 'a-z')
    want=$(printf '%s' "$1.exe" | tr 'A-Z' 'a-z')
    case "$out" in *"$want"*) return 0 ;; esac
    return 1
}

alive() { process_running "$1"; }

# Capture to a scratch file and promote only on success, and bank frames *during* the run.
#
# Two reasons, both learned the hard way. The pipe: `screenshot.sh` drives `powershell.exe`,
# and `... | grep -q captured` closes the pipe on the first match - the exact deadlock D194 is
# about, sitting in the sweep's own success path. `$(...)` reads to EOF instead.
#
# The frame: the window is the report for any loader whose text channel does not work, and a
# single shot at the end of a run is a shot taken at the one moment most likely to fail. A run
# long enough to be inconvenient gets minimised, a minimised window has no client area to
# capture, and the report is then lost for a run that completed perfectly. So a frame is banked
# early and replaced only by a *later successful* one; a failed capture leaves the last good
# frame standing rather than truncating it.
# The end record can be in either channel, so both are asked.
#
# Kyty's guest output never reaches the log - it goes to the report file it holds open - so a
# detector reading only the log never sees `OBS|end` and burns the whole budget on a run that
# finished in seconds. Its display is no help either: it presents at about 0.78 fps, so the
# screen was still drawing section 4 of 27 sixty seconds after the report said complete.
#
# Reading the held-open file may fail with "Device or resource busy", which is why this tries
# rather than tests, and why the mtime-quiet detector below stays as the fallback.
report_ended() {
    # The watched file must be newer than the stamp before a single byte of it is believed.
    #
    # It is the resume state, deliberately preserved between runs (D172), so at the moment a
    # run starts it still holds the *previous* run's `OBS|end`. Reading it unguarded declared
    # every Kyty run complete at zero seconds - before the window had even opened, so there was
    # not even a screenshot to contradict it. A detector that reports success instantly is
    # worse than one that never fires: the row said `complete` and the count was real, because
    # both came from the run before.
    { tr -d '\000' < "$1" | grep -q 'OBS|end'; } 2>/dev/null && return 0
    [ -n "$2" ] || return 1
    [ "$2" -nt "$work/stamp" ] 2>/dev/null || return 1
    # Braces, not a trailing `2>/dev/null`: bash applies redirections left to right, so a
    # failed `< "$2"` on the held-open file is reported by the shell *before* the stderr
    # redirect takes effect, and "Device or resource busy" lands in the terminal every second.
    { tr -d '\000' < "$2" | grep -q 'OBS|end'; } 2>/dev/null
}

capture() {
    dest="$2"
    out=$(sh "$(dirname "$0")/screenshot.sh" --process "$1" --out "$dest.new" --delay 0 2>&1 || true)
    case "$out" in
        *captured*)
            printf '%ss ok\n' "$elapsed" >> "$dest.capturelog"
            mv -f "$dest.new" "$dest" 2>/dev/null && return 0 ;;
    esac
    # Why a capture failed is worth keeping. A run can complete perfectly and still hand back
    # an early frame, and the difference between "the window went away" and "the window stopped
    # answering" decides whether that is fixable.
    printf '%ss FAILED: %s\n' "$elapsed" "$(printf '%s' "$out" | tr '\n' ' ' | cut -c1-160)" \
        >> "$dest.capturelog"
    rm -f "$dest.new" 2>/dev/null
    return 1
}



# Kyty takes a Lua configuration naming the module, the mount and which symbol tables to load.
write_kyty_lua() {
    cat > "$work/kyty.lua" <<LUA
local cfg = {
	ScreenWidth = 1280; ScreenHeight = 720; Neo = true;
	VulkanValidationEnabled = false; ShaderValidationEnabled = false;
	ShaderOptimizationType = 'Performance'; ShaderLogDirection = 'Silent';
	CommandBufferDumpEnabled = false;
	PrintfDirection = 'File'; PrintfOutputFile = '$(win "$work/kyty-printf.txt" | sed 's|\\|/|g')';
	ProfilerDirection = 'None';
}
kyty_init(cfg)
kyty_mount('$(win "$work" | sed 's|\\|/|g')', '/app0')
kyty_load_elf('/app0/eboot.bin')
for _, lib in ipairs({'libc_internal_1','libkernel_1','libVideoOut_1','libSysmodule_1',
                      'libDiscMap_1','libGraphicsDriver_1','libUserService_1','libSystemService_1',
                      'libPad_1','libAudio_1','libSaveData_1','libAppContent_1','libDialog_1',
                      'libNet_1','libPlayGo_1','libDebug_1'}) do
	kyty_load_symbols(lib)
end
kyty_execute()
LUA
}

printf '%-11s %8s %7s %9s  %s\n' loader records ended seconds outcome
printf '%.0s-' 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0; echo

echo "$LOADERS" | while IFS='|' read -r key exe flag proc gen; do
    [ -z "$key" ] && continue
    if [ -n "$WANTED" ]; then
        case " $WANTED " in *" $key "*) ;; *) continue ;; esac
    fi
    if [ ! -f "$exe" ]; then
        printf '%-11s %8s %7s %9s  %s\n' "$key" - - - "not built: $exe"
        continue
    fi

    stop "$proc"
    # Staged under the conventional name whatever it was called on disk: these loaders look
    # for a package layout around the executable, and a differently-named file is a variable
    # that would be indistinguishable from a loader bug.
    cp "$work/module-gen$gen.elf" "$work/eboot.bin"
    log="$OUT/$key.log"
    watch=""
    : > "$log"

    if [ "$flag" = "@lua" ]; then
        write_kyty_lua
        # Kyty resolves a relative report path against its own working directory, so the run
        # happens there and the report is collected from there afterwards.
        kdir=$(dirname "$exe")
        # The previous report is deliberately left in place: it *is* the resume state.
        #
        # obSCEne reads it before the sink truncates, finds the check that announced itself and
        # never returned, and skips that one. So consecutive runs of one binary walk past one
        # blocker each - which is how shadPS4 gets from 142 records to the whole suite in four
        # runs. Deleting it here made every Kyty run start from nothing and stop in the same
        # place forever.
        # Watched instead of the log, because Kyty's guest output never reaches the log.
        #
        # Its records go to this file through `sceKernelWrite`, so the end-of-report detector
        # below found nothing in the log and every Kyty run waited out the whole budget - two
        # minutes for a suite that finishes in five seconds.
        watch="$kdir/obscene-report.txt"
        # Stamped, because the file being watched is also the file being *kept*.
        #
        # It is the resume state, so it cannot be deleted before the run - and it still holds
        # the previous run's `end` record, which the detector below happily found within a
        # second of launching. Every Kyty run then reported "complete in 0s" on the strength of
        # the last one. A marker file written now makes "newer than this" the test, so only a
        # report this run produced counts.
        : > "$work/stamp"
        ( cd "$kdir" && "./$(basename "$exe")" "$(win "$work/kyty.lua")" ) >"$log" 2>&1 &
    else
        # Unquoted deliberately: an empty flag must vanish rather than become an empty
        # argument, which these loaders read as a path and refuse.
        # shellcheck disable=SC2086
        "$exe" $flag "$(win "$work/eboot.bin")" >"$log" 2>&1 &
    fi

    # Wall-clock, not an iteration count.
    #
    # This loop used to count its own turns and call the total seconds, which held only while
    # the body was one `grep` on a local file. Reading the clock costs nothing and cannot
    # drift, so the timeout keeps meaning seconds however the body grows. (D178)
    started=$(date +%s); elapsed=0; ended=no; quiet=0; last_write=""; last_size=""
    # Liveness is throttled, and that is not an optimisation.
    #
    # `alive` shells out to `tasklist`, which costs about two seconds warm and eighty-five cold.
    # Called every second it does not slow the loop down, it *breaks the budget*: the elapsed
    # test happens at the top, so one slow body can overshoot by more than the whole timeout.
    # Measured, with TIMEOUT=100: runs of 278s, 352s and 216s, and a Kyty window sitting on
    # screen for four minutes.
    #
    # `kill -0` is free and answers for loaders whose process id is real; only when that fails
    # does this reach for `tasklist`, and then at most once every ten seconds. Exactly the
    # throttle `run-emulator.sh` already carries - written there this morning and not carried
    # across, which is the same not-applying-a-known-fix that D178 is about. (D178)
    PROBE_SECONDS=10
    probe_at=0
    emulator_alive=1
    SHOT_SECONDS=${SHOT_SECONDS:-20}
    shot_at=5
    shot="-"
    while [ "$elapsed" -lt "$TIMEOUT" ]; do
        # Unanchored: records arrive embedded in a loader's own log lines rather than at the
        # start of one, and NUL bytes in fpPS4's output make `grep` call the stream binary and
        # print nothing at all - which reads as "no records" on a run that produced plenty.
        if report_ended "$log" "$watch"; then ended=yes; break; fi
        # Output stopping is a result, and waiting past it buys nothing.
        #
        # A loader that hangs inside a check writes nothing more, ever - but the budget goes on
        # running, so fpPS4 spent the full two minutes motionless on screen for each of the
        # forty-four checks it has to walk past. The report is the progress signal, so an
        # unchanging log means the guest has stopped, and the only question left is whether it
        # stopped because it finished or because it hung. The `end` record above answers that.
        #
        # Ten seconds, not five: the log path is shared by every loader, and cutting a check
        # short is not a neutral mistake - a run killed mid-check leaves a dangling `try`, which
        # the resume mechanism then skips permanently on a false signal (D181). Ten is longer
        # than any check here goes silent for, and still twelve times faster than the budget.
        size=$(wc -c < "$log" 2>/dev/null || echo 0)
        if [ "$size" = "$last_size" ] && [ "$size" != "0" ]; then
            quiet=$((quiet + 1))
            if [ "$quiet" -ge 10 ]; then break; fi
        else
            quiet=0
            last_size="$size"
        fi
        # For a loader whose report cannot be read while it runs, the end signal is the
        # report going *quiet*.
        #
        # Kyty holds its report file open, so every attempt to read it during a run comes back
        # "Device or resource busy" - the record is there, and unreachable. Its metadata is not:
        # the modification time advances with each write. So the test is "this run has written
        # something, and has now stopped writing for a few seconds", which needs no cooperation
        # from the loader and is not specific to this one.
        #
        # Five seconds because the suite writes continuously while it runs; a gap that long
        # means it is cycling result pages, which is what it does when finished.
        if [ -n "$watch" ] && [ "$watch" -nt "$work/stamp" ]; then
            now=$(date -r "$watch" +%s 2>/dev/null || echo 0)
            if [ "$now" = "$last_write" ]; then
                quiet=$((quiet + 1))
                if [ "$quiet" -ge 5 ]; then ended=yes; break; fi
            else
                quiet=0
                last_write="$now"
            fi
        fi
        if [ "$elapsed" -ge "$probe_at" ]; then
            if alive "$proc"; then emulator_alive=1; else emulator_alive=0; fi
            probe_at=$((elapsed + PROBE_SECONDS))
        fi
        if [ "$emulator_alive" != "1" ]; then break; fi
        if [ "$elapsed" -ge "$shot_at" ]; then
            if capture "$proc" "$OUT/$key.png"; then shot="$key.png"; fi
            shot_at=$((elapsed + SHOT_SECONDS))
        fi
        sleep 1
        elapsed=$(( $(date +%s) - started ))
    done
    elapsed=$(( $(date +%s) - started ))

    # Photographed before stopping it, because the window is the report for any loader whose
    # text channel does not work.
    # A last frame if the window is still there; the banked one stands if it is not.
    if alive "$proc"; then
        if capture "$proc" "$OUT/$key.png"; then shot="$key.png"; fi
    fi
    stop "$proc"

    # Kyty writes its report beside its executable rather than to the log, so it is collected
    # here - and every part of this line is defensive on purpose.
    #
    # `stop` has just killed the process and Windows does not release the handle instantly, so
    # a `cat` issued immediately gets "Device or resource busy". With `set -e` and no `|| true`
    # that failure ended the whole sweep silently: the loader ran, produced a full report, was
    # photographed, and then took the script down before printing its row. The two loaders
    # after it never ran at all, and nothing said why.
    if [ "$flag" = "@lua" ]; then
        sleep 1
        cat "$(dirname "$exe")/obscene-report.txt" >>"$log" 2>/dev/null || true
    fi
    # The watched file is part of the report, not an alternative to it.
    #
    # Kyty's guest records never reach the log - they go to the file it holds open for the
    # whole run, unreadable until the process is gone. Extracting from `$log` alone printed
    # `0 records` for a run whose own screenshot read SUITE COMPLETE, 515 of 515. That is the
    # worst shape a wrong number can take: zero reads as a dead loader, so the row said the
    # opposite of what happened and looked like a measurement while doing it.
    { tr -d '\000' < "$log" 2>/dev/null
      if [ -n "$watch" ] && [ -r "$watch" ]; then tr -d '\000' < "$watch" 2>/dev/null; fi
    } | grep -o 'OBS|.*' | sort -u > "$OUT/$key.txt" || true

    n=$(grep -c '^OBS|' "$OUT/$key.txt" 2>/dev/null || echo 0)
    last=$(grep 'OBS|try' "$OUT/$key.txt" | tail -1 | cut -d'|' -f3)

    if [ "$ended" = yes ]; then outcome="complete, shot=$shot"
    elif [ "$n" = "0" ]; then outcome="no records, shot=$shot"
    else outcome="stopped in ${last:-?}, shot=$shot"; fi
    tree=$(echo "$SOURCES" | grep "^$key|" | cut -d'|' -f2)
    printf '%-11s %8s %7s %9s  %s%s\n' "$key" "$n" "$ended" "${elapsed}s" "$outcome" \
        "$([ -n "$tree" ] && patched "$tree")"
done

echo
echo "logs, reports and screenshots: $OUT"
