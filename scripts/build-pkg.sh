#!/bin/bash
# Assemble obscene.pkg from the eboot, using selfish for every format step.
#
# This is obSCEne's orchestration; selfish owns the formats. `selfish image` now exists, so
# the pipeline runs end to end.
#
#   app0/eboot.bin
#   (no param.json - see the staging note below)
#        -> pfs image (selfish-pfs::build)
#        -> package    (selfish pack --image ... --out obscene.pkg)
set -e

BUILD="${1:?usage: build-pkg.sh <BUILD>}"
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
selfish() { ( cd "$SELFISH" && cargo run -q -p selfish-cli -- "$@" ); }

image="$BUILD/obscene.pfs.img"
out="$BUILD/obscene.pkg"

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
# Entry 0x200, the entry name table, generated rather than left empty.
#
# A NUL-separated list of the *named* entries' filenames, in entry-id order, opening with a NUL.
# It was empty here, which leaves a console unable to name the five title entries it is about to
# read. Byte-for-byte from a real package, where it is 75 bytes and reads:
#
#   \0 icon0.png \0 param.sfo \0 playgo-chunk.dat \0 playgo-chunk.sha \0 playgo-manifest.xml \0
#
# The order matches the entry ids that carry names: 0x1200, 0x1000, 0x1001, 0x1002, 0x1003.
printf '\0icon0.png\0param.sfo\0playgo-chunk.dat\0playgo-chunk.sha\0playgo-manifest.xml\0' \
    > "$ent/names.bin"
echo "build-pkg: names.bin = $(stat -c %s "$ent/names.bin") bytes"

# Entry 0x1001, the playgo chunk descriptor - the **full** structure, not just the header.
#
# The first version here wrote only the `plgo` header (chunk_count=1, scenario_count=1) and
# zeroed the rest. A console read that, found the counts promised a chunk and a scenario that the
# body did not describe, and `scePlayGoCoreGetRawContentInfo` returned `0x80f00200` after the
# header passed. This writes the whole thing, following `LibOrbisPkg`'s `ChunkDat.FromProject`:
# a header, a table of sub-table offsets at 0xC0, then chunk/mchunk/scenario records and their
# labels. The outer mchunk size is the package size; the offsets and records are fixed for a
# single-chunk title. (LibOrbisPkg PlayGo/ChunkDat.cs; oracle-checked)
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

# STEP 2 - wrap the image as a package.
#
# --title-id and --title feed the generated param.sfo. They are the title's identity, so they
# belong to obSCEne rather than to a format library, but the *encoding* is selfish's job now -
# what used to go in here was the word PLACEHOLDER, which a console rejects on the magic.
# obSCEne's own mark on the home screen, rather than selfish's.
#
# selfish's default icon is deliberately identifiable - it says "selfish built this and nobody
# supplied artwork", which is exactly what you want to know about a package you are debugging.
# This project *does* have artwork, so it supplies it and the default stops applying.
#
# The PNG is handed over as authored. `selfish pack` converts it to the 512x512 RGB a console
# wants - an icon with an alpha channel is accepted by a console and then composited differently,
# so the tile reads square beside every other icon on the home screen. Doing that conversion here
# would be this project keeping its own copy of a console requirement. (selfish D073)
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
    --entry "0x200=$ent/names.bin" \
    --entry "0x1001=$ent/playgo-chunk.dat" \
    "${icon_arg[@]}"

echo "build-pkg: wrote $out"
