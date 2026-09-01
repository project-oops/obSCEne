# The build environment, in one place.
#
# Sourced, not executed. Every script that used to drive a multipass VM sources this instead:
# multipass was dropped on 2026-08-26 (see CLAUDE.md for why, and D198 for what the mount
# costs), and five scripts were left calling a command that no longer exists. They did not
# fail loudly - `multipass` simply is not there, so they died on the first call with a message
# about a missing program rather than about a decommissioned build environment.
#
# # Why a shim rather than rewriting the call sites
#
# These scripts were written against the multipass call shapes, and those shapes carry meaning
# worth keeping: `--working-directory` says "run this in the repository", `transfer` says "get
# this file out". Rewriting forty call sites would have been forty chances to change one of
# them by accident. Translating the four shapes in one place is one chance, and it is here.
#
# # The rules that carry over from multipass, unchanged
#
# `MSYS_NO_PATHCONV=1` on every call. Git Bash rewrites anything that looks like a Unix path
# before a Windows program sees it, so `-d Ubuntu` survives but `$REPO` arrives as
# `C:/Program Files/Git$REPO` and the command fails on a directory that never
# existed. That is not a multipass quirk; it is Git Bash, and `wsl.exe` is just as Windows a
# program as `multipass.exe` was.
#
# `BUILD` must stay Linux-local - `$HOME/obs`, never `/mnt/c/...`. A Windows mount cannot carry
# the execute bit, and building across it is between six and forty times slower. (D012, D198)

# Paths are derived from this script's own location rather than hardcoded, so the
# collection works wherever it is cloned. `$OOPS` is the parent holding all four projects.
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
OOPS="$(cd "$REPO/.." && pwd)"
WSL_DISTRO="${WSL_DISTRO:-Ubuntu}"
# The repository, as WSL sees it. Under multipass this was a copy inside the VM at
# /home/ubuntu/obscene and had to be kept in sync; under WSL it is the same files.
VM_REPO="${VM_REPO:-$REPO}"

# A Windows path as WSL spells it. `cygpath -m` first because it knows the real mount table;
# the sed fallback covers a plain `/c/...` when cygpath is absent.
wslpath_of() {
    if command -v cygpath >/dev/null 2>&1; then
        _p=$(cygpath -m "$1")
        _drive=$(printf '%s' "$_p" | cut -c1 | tr 'A-Z' 'a-z')
        printf '/mnt/%s%s' "$_drive" "$(printf '%s' "$_p" | cut -c3-)"
    else
        printf '%s' "$1" | sed 's|^/\([a-zA-Z]\)/|/mnt/\1/|'
    fi
}

# Translates the multipass shapes these scripts already use:
#
#   vm exec NAME [--working-directory DIR] -- bash -lc "CMD"
#   vm transfer NAME:SRC DEST
#
# Anything else is refused rather than guessed at, because a shim that silently does something
# approximate to a command it did not recognise is worse than one that stops.
vm() {
    case "$1" in
        exec)
            shift 2                       # drop `exec` and the instance name
            _dir=""
            if [ "$1" = "--working-directory" ]; then
                _dir="$2"
                shift 2
            fi
            [ "$1" = "--" ] && shift
            # The remaining words are the command. `bash -lc "..."` is the only form these
            # scripts use, and it is the one that needs a login shell - `$HOME` appears inside
            # build paths and a bare command gets no shell to expand it.
            if [ "$1" = "bash" ] && [ "$2" = "-lc" ]; then
                if [ -n "$_dir" ]; then
                    MSYS_NO_PATHCONV=1 wsl.exe -d "$WSL_DISTRO" -- bash -lc "cd '$_dir' && $3"
                else
                    MSYS_NO_PATHCONV=1 wsl.exe -d "$WSL_DISTRO" -- bash -lc "$3"
                fi
            else
                MSYS_NO_PATHCONV=1 wsl.exe -d "$WSL_DISTRO" -- "$@"
            fi
            ;;
        transfer)
            shift
            _src=${1#*:}                  # strip the `NAME:` prefix
            _dst=$2
            # WSL writes straight to the Windows filesystem through /mnt, so there is no
            # transfer step and nothing to misreport. multipass used to copy the file
            # correctly and *then* fail setting POSIX permissions on NTFS - reporting failure
            # having done the job, which is why every call site ignores the exit code and
            # tests for the file instead. Those tests are still right and still cheap.
            MSYS_NO_PATHCONV=1 wsl.exe -d "$WSL_DISTRO" -- bash -lc \
                "cp \"$_src\" '$(wslpath_of "$_dst")'"
            ;;
        *)
            echo "vm(): unsupported multipass verb '$1' - see scripts/wsl.sh" >&2
            return 2
            ;;
    esac
}
