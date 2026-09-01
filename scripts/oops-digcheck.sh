#!/bin/bash
# Does the package still agree with itself?
#
# Every header field selfish now writes was measured against real packages, but several of them
# (`sc_entry_count`, `main_entry_data_size`, `promote_size`, the manifest's fixed word and its
# digests) are written *near* the regions the package hashes. A digest computed before a field
# changed is a package that describes itself wrongly - and a console that reads it reports an
# error reading application data, which is exactly what CE-108262-9 says.
#
# So: recompute every digest the package claims and compare. Entry 0x1 is the digest table, one
# SHA-256 per entry in table order with its own slot zeroed; entry 0x80 holds the image digest at
# 0x40 and the param.sfo digest at 0xC0.
#
#     oops-digcheck.sh [package]        (default: the staged build)
set -u
f="${1:-$HOME/obs-pkg/obscene.pkg}"
[ -f "$f" ] || { echo "no package at $f" >&2; exit 1; }

be32() { od -An -t u4 -j "$1" -N 4 --endian=big "$f" | tr -d ' '; }
n=$(be32 16)
toff=$(be32 24)

# Locate the digest table (entry 0x1) and the manifest (entry 0x80).
dt=""; man=""
for i in $(seq 0 $((n - 1))); do
    off=$((toff + i * 32))
    id=$(be32 $off)
    eo=$(be32 $((off + 16)))
    [ "$id" = 1 ] && dt=$eo
    [ "$id" = 128 ] && man=$eo
done

echo "=== $(basename "$f"): $n entries, table at $toff, digest table at ${dt:-?} ==="
good=0; bad=0
for i in $(seq 0 $((n - 1))); do
    off=$((toff + i * 32))
    id=$(be32 $off)
    eo=$(be32 $((off + 16)))
    es=$(be32 $((off + 20)))
    # The digest table's own slot is zero by construction; skip it rather than report it wrong.
    [ "$id" = 1 ] && continue
    stored=$(dd if="$f" bs=1 skip=$((dt + i * 32)) count=32 2>/dev/null | od -An -tx1 | tr -d ' \n')
    actual=$(dd if="$f" bs=1 skip="$eo" count="$es" 2>/dev/null | sha256sum | cut -d' ' -f1)
    if [ "$stored" = "$actual" ]; then
        good=$((good + 1))
    else
        bad=$((bad + 1))
        printf '  MISMATCH entry 0x%x\n    stored %s\n    actual %s\n' "$id" "$stored" "$actual"
    fi
done
echo "  entry digests: ok=$good bad=$bad"

# The manifest's two computable digests.
if [ -n "$man" ]; then
    imgoff=$(od -An -t u8 -j 1040 -N 8 --endian=big "$f" | tr -d ' ')
    stored=$(dd if="$f" bs=1 skip=$((man + 0x40)) count=32 2>/dev/null | od -An -tx1 | tr -d ' \n')
    actual=$(dd if="$f" bs=1 skip="$imgoff" 2>/dev/null | sha256sum | cut -d' ' -f1)
    [ "$stored" = "$actual" ] \
        && echo "  manifest[0x40] image digest: ok" \
        || printf '  manifest[0x40] image digest: MISMATCH\n    stored %s\n    actual %s\n' "$stored" "$actual"

    for i in $(seq 0 $((n - 1))); do
        off=$((toff + i * 32))
        [ "$(be32 $off)" = 4096 ] || continue
        eo=$(be32 $((off + 16))); es=$(be32 $((off + 20)))
        stored=$(dd if="$f" bs=1 skip=$((man + 0xC0)) count=32 2>/dev/null | od -An -tx1 | tr -d ' \n')
        actual=$(dd if="$f" bs=1 skip="$eo" count="$es" 2>/dev/null | sha256sum | cut -d' ' -f1)
        [ "$stored" = "$actual" ] \
            && echo "  manifest[0xc0] param.sfo digest: ok" \
            || printf '  manifest[0xc0] param.sfo digest: MISMATCH\n    stored %s\n    actual %s\n' "$stored" "$actual"
    done
fi

# The header's own size fields, which must describe the file that exists.
size=$(stat -c %s "$f")
pkgsize=$(od -An -t u8 -j 1072 -N 8 --endian=big "$f" | tr -d ' ')
imgoff=$(od -An -t u8 -j 1040 -N 8 --endian=big "$f" | tr -d ' ')
imgsize=$(od -An -t u8 -j 1048 -N 8 --endian=big "$f" | tr -d ' ')
echo "  file $size | package_size $pkgsize | image_offset $imgoff + image_size $imgsize = $((imgoff + imgsize))"
[ "$size" = "$pkgsize" ] && echo "    package_size agrees with the file" || echo "    package_size DISAGREES with the file"
[ "$size" = "$((imgoff + imgsize))" ] && echo "    image spans to end of file" || echo "    image does NOT span to end of file"
