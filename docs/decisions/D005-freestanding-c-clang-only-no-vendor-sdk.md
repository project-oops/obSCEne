# D005 - Freestanding C, clang only, no vendor SDK

**assumed** · 2026-08-19

`-ffreestanding -nostdlib`, target triple `x86_64-unknown-freebsd`, no libc, no SDK
headers. Imports are ordinary undefined symbols the loader resolves.

Three things fall out of it. Provenance stays clean - nothing here derives from
vendor headers. The import list in the finished object is an accurate statement of
what the program needs, because nothing was linked in that a console would not
already provide. And the build needs one tool that anybody already has.

`memcpy` and `memset` are defined by hand: the compiler emits calls to them
regardless of `-ffreestanding`, and without definitions the link fails naming a
function nothing in the source calls.

