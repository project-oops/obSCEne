#!/bin/bash
# Build the native title and push it to a scan root on the target (default /user/data/<TITLE_ID>/)
# via prosperous, where an auto-mounter registers it into /user/app.
#
# The native counterpart to `deploy` (which builds and installs the package). A native title is a
# directory, so this uploads the directory - eboot.bin + sce_sys/{param.json,icon0.png} - with
# `obscene-tool hw install-native`, which is prosperous's `transfer::upload` (an FTP STOR per file,
# directories made as needed). All out-connections, so it stays in WSL - unlike the package
# `hw install`, which the console must connect *in* to fetch.
#
# It lands the directory in a scan root (default /user/data) - one of the directories an
# auto-mounter (ShadowMountPlus) scans and registers into /user/app for you. It does not touch the
# app database itself. Note /user/app is NOT scanned, so it is not the target.
#
# Flags: --deploy-only pushes an already-built dir without rebuilding; --into <path> targets a
# different scan root (e.g. --into /mnt/usb0); --name desk picks a non-default device. All pass
# through to the tool.
set -euo pipefail
[ -f "$HOME/.cargo/env" ] && source "$HOME/.cargo/env"
cd "$(dirname "$0")/.."

BUILD="${BUILD:-$HOME/obs}"

build=1
if [ "${1:-}" = "--deploy-only" ]; then build=0; shift; fi

# The title id, from the one place it lives (data/identity.toml), derived the same way the builds do.
cid="$(sed -n 's/^content_id_native[[:space:]]*=[[:space:]]*"\(.*\)"[[:space:]]*$/\1/p' data/identity.toml)"

if [ "$build" = 1 ]; then
    make native BUILD="$BUILD"
fi

dir="$BUILD/native/$appid"
[ -d "$dir" ] || {
    echo "native-deploy: no title dir at $dir (build first, or drop --deploy-only)" >&2
    exit 1
}

# The release tool, built if absent - the same binary bin/obscene's helper uses.
[ -x tool/target/release/obscene-tool ] || ( cd tool && cargo build --release --quiet )
./tool/target/release/obscene-tool hw install-native "$dir" "$@"
echo "native-deploy: $appid uploaded to a scan root - ShadowMountPlus registers it from there"
