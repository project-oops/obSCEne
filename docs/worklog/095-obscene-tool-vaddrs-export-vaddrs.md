# 2026-08-31 - `obscene-tool vaddrs`: export vaddrs resolved to names


Added a subcommand that reads a module's exports through SELFish and resolves each NID to a name
from the mined corpus, emitting the `name vaddr` table `orbistoun-firmware` reads. Generated
`orbistoun/crates/orbistoun-firmware/data/libkernel-vaddrs.txt` from the 12.40 `libkernel_sys.sprx`
- 1,867 exports, 1,644 named; orbistoun-firmware's own `libkernel_exports` test passes. The join
key was the surprise: the corpus records a NID as raw hex (`0x1e82d558d6a70417`) and an export
table records the base64 encoding (`HoLVWNanBBc`) - two forms of one value, bridged by
`selfish_nid::Nid::decode`. See D265. Unresolved NIDs keep their encoded form rather than being
dropped or guessed. Verified via the tool's own clippy `-D warnings` and unit tests: the C
`make check` host build is currently broken by a parallel session's `src/sections/selfdump.c`
(missing `obscene/imports.h`), which is not this change.

