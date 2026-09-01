# 2026-08-19 - Scaffolded, written, and building


**Done.** The whole first pass, from empty directory to two working builds.

- Harness: statuses, results, checks, sections, capability-based dependencies.
- Freestanding runtime: write path, decimal/hex/signed formatting, `memcpy`/`memset`.
- Report emitter and the pipe-separated format, with `try` written before every call.
- 35 checks across 11 sections, base to high level.
- Host stubs and a native entry point, so the harness runs with no emulator.
- Tooling: `pretty.py` (colour), `verify.py` (format invariants), `imports.py` (a
  small ELF reader, no binutils needed).
- Both builds clean under `-Werror -Wconversion -Wsign-conversion -Wshadow`.

**Verified.** `make check` passes: target module builds with entry point resolving to
`obscene_start` and **30 imports**; host build runs 35 checks producing 16 pass, 4
partial, 7 fail, 8 skip; `verify.py` reports the stream well-formed.

**Surprises.**

- **The synthetic error sentinel was wrong in exactly the way its own comment said it
  would not be.** It was defined as `0x7F000001` with the high bit deliberately clear
  so it could never be mistaken for a real platform code, then returned *negated*.
  `-0x7F000001` is `0x80FFFFFF` - the exact shape of a real error code. The comment
  reasoned about the constant; the caller sees the returned value. Caught on the first
  host run and completely invisible in the source. (D011)

- **The host build earned its place immediately.** It exists so a bug in the harness
  can be told apart from a bug in the emulator, and it justified itself before any
  emulator was involved at all, by exposing the above.

- **`CC ?= clang` silently does nothing in a Makefile.** Make defines `CC = cc` as a
  built-in, so `?=` sees it already set and leaves it. The first build ran under gcc
  without saying so. `CC :=` is correct; a command-line `make CC=...` still wins.

- **A Windows directory mounted into a Linux VM cannot carry the execute bit.** The
  host binary built fine into the tree and then refused to run with "Permission
  denied", which reads like a filesystem permissions problem and is actually a
  property of the mount. Building to a VM-local path is the fix. (D012)

- **`clang-format -i` destroys files on a mounted Windows share.** It writes in place
  by rename, and the mount refuses the rename after the temp file is written - leaving
  `src/host_stubs.c.temp-stream-bc1380` and no `src/host_stubs.c`. The content survived
  and the files were recoverable, but the next build failed with "no such file or
  directory" for a file that had existed a second earlier. Format by redirect instead:
  `clang-format "$f" > tmp && cat tmp > "$f"` truncates and writes in place with no
  rename. Same root cause as D012 - the mount is not a normal filesystem.

- **Two ordering assumptions were both wrong in the tests before they were right in
  the code.** The first registry definition used a static array plus alias pointers
  that did not match the extern declaration in the header; the second, direct one is
  both simpler and correct. Worth noting that the indirection was invented to solve a
  problem that did not exist.

**Not done.** Never run on real hardware, and never run under an emulator - the
loader it is meant for cannot jump to an entry point yet. Struct-taking functions
(`sceVideoOutRegisterBuffers`, `scePadReadState`, the GPU submission path) are
deliberately absent pending confident signatures (D008). The suite leans on negative
checks, and the ratio should shift as signatures become known (D007).

