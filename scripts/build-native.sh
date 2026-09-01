#!/bin/bash
# Lay out obSCEne as a **native title directory**, the current generation's own title format.
#
# This is not a second package format. `make pkg` produces a previous-generation package, which
# installs through the compatibility path and is badged as the older hardware - that is not a
# labelling quirk, it is what the package is. This target produces the other thing entirely:
#
#   <TITLE_ID>/sce_sys/param.json    what the title says about itself
#   <TITLE_ID>/sce_sys/icon0.png     the home-screen tile
#   <TITLE_ID>/eboot.bin             the fake-signed fSELF the title launches
#
# The directory is registered under /user/app/<TITLE_ID>/ by sceAppInstUtilAppInstallTitleDir
# (kernel-privileged, so a payload does it - or an existing auto-mounter such as ShadowMountPlus
# picks the directory up from storage and registers it). That registration is what produces a
# **current-generation** home-screen entry, where a package would be badged as the older one.
#
# ## Why the eboot is here now, when an earlier version left it out
#
# This used to lay out param.json + icon and *no* executable, on the reasoning that a native
# title that runs its own code needs a signed eboot and no fake-signing keyset exists for this
# generation. That is true off-jailbreak and false on the target: with kstuff active the kernel
# accepts a **fake-signed fSELF**, which is the same eboot the package already installs and
# launches. So the title carries it and launches its own code, the way the shipping PS5 homebrew
# loaders do - rather than only deeplinking to a payload. Set NO_EBOOT=1 for the older
# deeplink-only layout (a launcher entry with no executable of its own).
#
# The eboot itself is built by `make eboot` (this script is invoked by `make native`, which
# depends on it) and its bytes come from selfish - this script only orchestrates.
set -e
[ -f "$HOME/.cargo/env" ] && source "$HOME/.cargo/env"

BUILD="${1:?usage: build-native.sh <BUILD>}"
SELFISH="${SELFISH:-../selfish}"
# obSCEne's own identity, read from the one place it lives (data/identity.toml). The native title has
# its OWN id (content_id_native), distinct from the package's, so the two can be installed side by
# side without colliding (D292). The current-generation status comes from the native registration
# path (AppInstallTitleDir) plus param.json, not from the id's prefix or the eboot's container magic.
#
# The content id is the single fact; the title id is derived from it exactly as build-pkg.sh does,
# so the two builds cannot disagree. Override CONTENT_ID for the stuck-title case (identity.toml, D223).
identity="$(dirname "$0")/../data/identity.toml"
toml_str() { sed -n "s/^$1[[:space:]]*=[[:space:]]*\"\(.*\)\"[[:space:]]*\$/\1/p" "$identity"; }
# The native title has its own id (content_id_native), distinct from the package's, so the two can
# be installed side by side without colliding. build-pkg.sh reads content_id; this reads its own.
CONTENT_ID="${CONTENT_ID:-$(toml_str content_id_native)}"
TITLE="${TITLE:-$(toml_str title)}"
if [ -z "${TITLE_ID:-}" ]; then
    t="${CONTENT_ID#*-}"
    TITLE_ID="${t%%_*}"
fi
deeplink_arg=()
category_arg=(--category 0)
if [ "${NO_EBOOT:-0}" = "1" ]; then
    DEEPLINK="${DEEPLINK:-http://127.0.0.1:8080/}"
    deeplink_arg=(--deeplink "$DEEPLINK")
    category_arg=(--category 65536)
fi

out="$BUILD/native"
rm -rf "$out"
mkdir -p "$out"

# Stage what `selfish native --root` copies verbatim into the title directory: the eboot, unless
# a deeplink-only layout was asked for.
root_arg=()
if [ "${NO_EBOOT:-0}" != "1" ]; then
    if [ ! -f "$BUILD/eboot.bin" ]; then
        echo "build-native: no eboot at $BUILD/eboot.bin - run 'make native' (which builds it) or" >&2
        echo "              'make eboot' first, or set NO_EBOOT=1 for a deeplink-only layout." >&2
        exit 1
    fi
    ebootroot="$BUILD/native-root"
    rm -rf "$ebootroot"
    mkdir -p "$ebootroot"
    cp "$BUILD/eboot.bin" "$ebootroot/eboot.bin"
    if [ -d "$BUILD/sce_module" ]; then
        cp -r "$BUILD/sce_module" "$ebootroot/sce_module"
    fi
    root_arg=(--root "$(cd "$ebootroot" && pwd)")
fi

selfish() { ( cd "$SELFISH" && cargo run -q -p selfish-cli -- "$@" ); }

icon_arg=()
if [ -f assets/logo.png ]; then
    icon_arg=(--icon "$(pwd)/assets/logo.png")
fi

selfish native \
    --out "$(cd "$out" && pwd)" \
    --title-id "$TITLE_ID" \
    --title "$TITLE" \
    --content-id "$CONTENT_ID" \
    "${category_arg[@]}" \
    "${deeplink_arg[@]}" \
    "${root_arg[@]}" \
    "${icon_arg[@]}"

echo
echo "build-native: laid out $out/$TITLE_ID ($TITLE_ID)"
if [ "${NO_EBOOT:-0}" = "1" ]; then
    echo "build-native: deeplink-only - a launcher entry, no executable of its own (NO_EBOOT=1)."
else
    echo "build-native: self-contained - carries its fake-signed eboot; launches its own code under kstuff."
fi
echo "build-native: register by copying $out/$TITLE_ID to /user/app/ and calling"
echo "              sceAppInstUtilAppInstallTitleDir(\"$TITLE_ID\", \"/user/app/\", 0) from a payload,"
echo "              or drop it where an auto-mounter (ShadowMountPlus) scans and it registers itself."
