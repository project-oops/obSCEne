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
# The title identity comes from app.env.
app_env="$(dirname "$0")/../app.env"
# shellcheck source=/dev/null
[ -f "$app_env" ] && . "$app_env"
TITLE_CODE="${TITLE_CODE:-O00001}"
TITLE="${TITLE:-${TITLE_NAME:-obSCEne}}"
if [ -n "${CONTENT_ID:-}" ] && [ -z "${TITLE_ID:-}" ]; then
    t="${CONTENT_ID#*-}"
    TITLE_ID="${t%%_*}"
fi
TITLE_ID="${TITLE_ID:-ORB${TITLE_CODE}}"
CONTENT_ID="${CONTENT_ID:-IV0002-${TITLE_ID}_00-STOREUPD00000000}"

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
ent="$BUILD/pkg-entries"
rm -rf "$ent"
mkdir -p "$ent"
IMG_SIZE=$(stat -c %s "$image")
PKG_SIZE=$(( 0x80000 + IMG_SIZE )); INNER_SIZE=11141120
python3 - "$ent/playgo-chunk.dat" "$CONTENT_ID" "$PKG_SIZE" "$INNER_SIZE" <<'PY'
import sys, struct
out, cid, pkg_size, inner_size = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
b = bytearray(416)
def u16(o,v): struct.pack_into('<H', b, o, v)
def u32(o,v): struct.pack_into('<I', b, o, v)
def u64(o,v): struct.pack_into('<Q', b, o, v)
b[0:4] = b'plgo'
u16(0x08,1)          # image_count
u16(0x0A,1)          # chunk_count
u16(0x0C,1)          # mchunk_count
u16(0x0E,1)          # scenario_count
u32(0x10,416)        # file_size
u16(0x14,0)          # default_scenario_id
u16(0x16,1)          # attrib
b[0x20:0x40] = b'\xff'*32                       # reserved
b[0x40:0x40+len(cid)] = cid.encode()            # content id
# table of sub-table (offset,size) pairs at 0xC0
toc = [(256,32),(288,2),(304,9),(320,16),(352,32),(384,2),(400,12),(336,16)]
for i,(off,sz) in enumerate(toc):
    u32(0xC0+i*8, off); u32(0xC0+i*8+4, sz)
# 0x100 ChunkAttr: flag=0x80, layer=0, req_locus=3, mchunk_count=1, language_mask=all, offsets 0
b[0x100]=0x80; b[0x101]=0; b[0x102]=3
u16(0x100+0x0E,1); u64(0x100+0x10,0xFFFFFFFFFFFFFFFF); u32(0x100+0x18,0); u32(0x100+0x1C,0)
u16(0x120,0)                                    # chunk->mchunk map: [0]
b[0x130:0x130+8]=b'Chunk #0'                    # chunk label
u64(0x140,0); u64(0x148,pkg_size)               # mchunk[0]: offset 0, size = package size
u64(0x150,0); u64(0x158,inner_size)             # inner mchunk[0]: offset 0, size = inner size
b[0x160]=1                                       # scenario type=1
u16(0x160+0x14,1); u16(0x160+0x16,1); u32(0x160+0x18,0); u32(0x160+0x1C,0)
u16(0x180,0)                                     # scenario->chunk map: [0]
b[0x190:0x190+11]=b'Scenario #0'                 # scenario label
open(out,'wb').write(bytes(b))
PY
echo "build-pkg: playgo-chunk.dat = $(stat -c %s "$ent/playgo-chunk.dat") bytes, full structure (pkg=$PKG_SIZE)"

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
    --entry "0x1001=$ent/playgo-chunk.dat" \
    "${icon_arg[@]}"
echo "build-pkg: wrote $out"
