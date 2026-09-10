#!/bin/bash
# Assemble obscene-probe-orbis.pkg from the eboot, using selfish for every format step.
#
# This is obSCEne's orchestration; selfish owns the formats. `selfish image` now exists, so
# the pipeline runs end to end.
#
#   app0/eboot.bin
#   (no param.json - see the staging note below)
#        -> pfs image (selfish-pfs::build)
#        -> package    (selfish pack --image ... --out obscene-probe-orbis.pkg)
set -e

BUILD="${1:?usage: build-pkg.sh <BUILD> [TARGET]}"
TARGET_NAME="${2:-orbis}"
SELFISH="${SELFISH:-../selfish}"
GEN="${GEN:-4}"
# The title identity lives in one place, read by both this and build-native.sh (data/identity.toml),
# so the package and the native title are the same app rather than two copies of one id that drift.
identity="$(dirname "$0")/../data/identity.toml"
toml_str() { sed -n "s/^$1[[:space:]]*=[[:space:]]*\"\(.*\)\"[[:space:]]*\$/\1/p" "$identity"; }
CONTENT_ID="${CONTENT_ID:-$(toml_str content_id)}"
TITLE="${TITLE:-$(toml_str title)}"

# The title id is *inside* the content id, so it is taken from there rather than written twice.
#
# It was hardcoded below while the content id was overridable, which is two copies of one fact
# and only one of them moves. The licence is keyed to the content id and a package must declare
# the matching title id, so a `CONTENT_ID=` override that did not carry through produced a
# package the console rejects for a reason unrelated to anything being tested.
#
# Overriding the pair is also how a stuck title is worked around: a crashed process the console
# will not reap holds its title id, and every install and launch against it is refused
# (`checkExistingApp: 0x8094000c`). Building under a fresh id sidesteps that without a reboot,
# which on a jailbroken console is an hour. (D223)
TITLE_ID="${CONTENT_ID#*-}"
TITLE_ID="${TITLE_ID%%_*}"
case "$TITLE_ID" in
    ????[0-9][0-9][0-9][0-9][0-9]) ;;
    *)
        echo "build-pkg: CONTENT_ID $CONTENT_ID does not contain a title id" >&2
        exit 1
        ;;
esac

[ -f "$BUILD/eboot.bin" ] || { echo "build-pkg: no eboot at $BUILD/eboot.bin - run 'make eboot' first" >&2; exit 1; }

# Stage the app tree the installer expects.
#
# **No param.json.** It used to be written here, and no real package carries one - three
# extracted samples have exactly one file in sce_sys, and it is `keystone`. A package's title
# metadata is the `param.sfo` *entry*, which selfish generates.
#
# param.json is not wrong, it belongs to the other route. It is what a **native** title carries
# at /user/app/<TITLEID>/sce_sys/param.json, registered with
# sceAppInstUtilAppInstallTitleDir - the mechanism that produces a PS5-badged title. Putting
# one inside a PS4-format package mixed the two conventions and got the benefit of neither.
#
# The keystone is not staged here either: selfish derives it from the passcode and puts it in.
app="$BUILD/pkg-root"
rm -rf "$app"
mkdir -p "$app"
cp "$BUILD/eboot.bin" "$app/eboot.bin"

# The sce_module stubs, which the loader **requires**: an eboot whose /app0/sce_module is missing
# is refused with PRX_SCE_MODULE_LOAD_ERROR ("Lack of a .prx file in /app0/sce_module") before a
# single line of the probe runs. `make pkg` builds them into $BUILD/sce_module via the sce-module
# prerequisite; this stages them into the image. Dropping this copy was a real regression - the
# package built, installed, and died at launch on the missing libc.prx / libSceFios2.prx. (D267)
if [ -d "$BUILD/sce_module" ]; then
    cp -r "$BUILD/sce_module" "$app/sce_module"
else
    echo "build-pkg: no $BUILD/sce_module - the package will be refused with PRX_SCE_MODULE_LOAD_ERROR." >&2
    echo "build-pkg: run 'make sce-module' first (or 'make pkg', which depends on it)." >&2
    exit 1
fi

# Where selfish is invoked.
if [ -x "$SELFISH/target/release/selfish" ]; then
    selfish() { "$SELFISH/target/release/selfish" "$@"; }
else
    selfish() { ( cd "$SELFISH" && PATH="$HOME/.cargo/bin:$PATH" cargo run -q -p selfish-cli -- "$@" ); }
fi
image="$BUILD/obscene.pfs.img"
out="$BUILD/obscene-probe-${TARGET_NAME}.pkg"

# STEP 1 - the filesystem image.
#
# The content id is passed here as well as to `pack`, and it has to be the same string in both.
# The image is encrypted under a key derived from it, so a mismatch produces two files that
# each look fine and a package whose filesystem cannot be opened.
selfish image --root "$app" --out "$image" --content-id "$CONTENT_ID"

# What still has to be handed over.
#
# selfish computes or generates everything it has grounds for - both digest tables, the block
# digests, both licences, both key blobs, a real param.sfo, the default playgo manifest, and a
# blank icon. Two entries are left, and they are left deliberately:
#
#   0x200  the entry name table
#   0x1001 playgo-chunk.dat
#
# Empty rather than invented. If a console turns out to want either of them, that will show up
# as a specific rejection rather than as a wrong guess that got installed.
# STEP 2 - wrap the image as a package.
#
# --title-id and --title feed the generated param.sfo. They are the title's identity, so they
# belong to obSCEne rather than to a format library, but the *encoding* is selfish's job now.
# Entry 0x200 (names table) and entry 0x1001 (playgo chunk descriptor) are now generated
# automatically by selfish (REQ-20260910T0325Z-6b04).
#
# selfish's default icon is deliberately identifiable - it says "selfish built this and nobody
# supplied artwork", which is exactly what you want to know about a package you are debugging.
# This project *does* have artwork, so it supplies it and the default stops applying.
ICON="${ICON:-$(cd "$(dirname "$0")/.." && pwd)/assets/logo.png}"
icon_arg=()
if [ -f "$ICON" ]; then
    icon_arg=(--entry "0x1200=$ICON")
else
    echo "build-pkg: no icon at $ICON - the package will carry selfish own mark" >&2
fi

selfish pack --image "$image" --content-id "$CONTENT_ID" --out "$out" \
    --title-id "$TITLE_ID" \
    --title "$TITLE" \
    "${icon_arg[@]}"
echo "build-pkg: wrote $out"
