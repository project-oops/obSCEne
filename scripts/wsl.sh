# shellcheck shell=sh
# The build environment for the emulator scripts: `vm exec` and `vm transfer` run in WSL.
#
# Sourced by a script in this directory, never executed. Every wsl.exe call sets
# MSYS_NO_PATHCONV=1, or Git Bash rewrites Unix paths before wsl.exe sees them (D199). BUILD
# stays Linux-local (`$HOME/obs`): a Windows mount carries no execute bit (D012).

# `$0` is the sourcing script, which lives beside this one.
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
WSL_DISTRO="${WSL_DISTRO:-Ubuntu}"
# The repository as WSL sees it: the same files, through the mount.
VM_REPO="${VM_REPO:-$REPO}"

# A Windows path as WSL spells it. `cygpath -m` first because it knows the real mount table;
# the sed fallback covers a plain `/c/...` when cygpath is absent.
wslpath_of() {
    if command -v cygpath >/dev/null 2>&1; then
        _p=$(cygpath -m "$1")
        _drive=$(printf '%s' "$_p" | cut -c1 | tr '[:upper:]' '[:lower:]')
        printf '/mnt/%s%s' "$_drive" "$(printf '%s' "$_p" | cut -c3-)"
    else
        printf '%s' "$1" | sed 's|^/\([a-zA-Z]\)/|/mnt/\1/|'
    fi
}

# Runs one of two call shapes in WSL, and refuses anything else:
#
#   vm exec NAME [--working-directory DIR] -- bash -lc "CMD"
#   vm transfer NAME:SRC DEST
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
            # `bash -lc` gets a login shell so `$HOME` in build paths expands.
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
            # A copy through the /mnt mount; call sites test for the file afterwards.
            MSYS_NO_PATHCONV=1 wsl.exe -d "$WSL_DISTRO" -- bash -lc \
                "cp \"$_src\" '$(wslpath_of "$_dst")'"
            ;;
        *)
            echo "vm(): unsupported verb '$1' - see scripts/wsl.sh" >&2
            return 2
            ;;
    esac
}
