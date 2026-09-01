#!/bin/bash
# Build an obSCEne payload: obSCEne's crt0 + body, resolution table generated from the target
# libraries via obscene-tool (which consumes selfish's format primitives). selfish provides the
# knowledge (dynsym/nid/exports); the crt0 and this orchestration are obSCEne's runtime.
# Paths are derived from this script's own location rather than hardcoded, so the
# collection works wherever it is cloned. `$OOPS` is the parent holding all four projects.
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
OOPS="$(cd "$REPO/.." && pwd)"
SELFISH="${SELFISH:-$OOPS/SELFish}"
set -e
export PATH="$HOME/.cargo/bin:$PATH" CARGO_TARGET_DIR="$HOME/obs-tool-target"
cd "$REPO"
BODY="$1"; OUT="$HOME/obs-hw/$(basename "$BODY" .c)_payload.elf"
CRT0="$SELFISH/runtime/crt0.c"
CFLAGS="-std=c11 -Wall -fvisibility=hidden -target x86_64-unknown-freebsd -ffreestanding -fno-builtin -nostdlib -fPIC -fno-stack-protector -I$SELFISH/runtime/include"
LDFLAGS="-fuse-ld=lld -shared -Wl,-Bsymbolic -Wl,-e,_start -Wl,--unresolved-symbols=ignore-all -Wl,-z,noexecstack -Wl,-z,max-page-size=0x4000 -Wl,-z,common-page-size=0x4000"
GEN="cargo run --manifest-path tool/Cargo.toml --example gen_payload_table -q --"

clang $CFLAGS -c "$CRT0" -o /tmp/crt0.o
clang $CFLAGS -c "$BODY" -o /tmp/body.o
clang $CFLAGS $LDFLAGS /tmp/crt0.o /tmp/body.o -o /tmp/stage1.elf
$GEN /tmp/stage1.elf "$HOME/libkernel_sys.sprx" "${@:2}" > /tmp/table.c 2>/tmp/gen.err
tail -1 /tmp/gen.err
clang $CFLAGS -c /tmp/table.c -o /tmp/table.o
clang $CFLAGS $LDFLAGS /tmp/crt0.o /tmp/body.o /tmp/table.o -o "$OUT"
echo "built: $(stat -c %s "$OUT") bytes -> $OUT"
