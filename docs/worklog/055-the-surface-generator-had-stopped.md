# The surface generator had stopped working, and nothing was watching


`obscene-tool surface` replaces `gen-surface.py`, and the port found the generator already
broken: **it refused its own definition**. Ten names had been promoted to `platform.h` over
time - seven networking calls and this morning's three mutex-attribute ones - and each was
removed from `surface.h` and added to the refusal list while staying in the generator's own
group list. Running it printed `scePthreadMutexattrInit is declared in platform.h and cannot
also be censused` and wrote nothing.

Nobody knew because it was never gated. `verify.sh` gates it now, which is the part that
matters more than the port: a generator whose output is committed and whose input nobody runs
is a generator that has already diverged, you just cannot see it yet.

**The committed header also carried a note the generator never had.** A four-line comment
explaining that the networking names moved to `platform.h` was hand-added to `surface.h`, so
even a working generator would have deleted it. Restored into the data file, where it belongs.

The definition moved to `data/surface.txt` for the same reason the GPU table did: seventeen
groups, several pages of prose and 371 names are data, and retyping data into a new language
is how a transcription error gets into a census. `surface.h` now regenerates byte-for-byte.

Getting there took three formatting corrections, all of the same kind - **whitespace nobody
can see is still content.** A blank line between a note and its macro; a trailing newline on
a prose block that split `#define OBS_SURFACE_LIBRARIES(L) \` across two lines; and a block
separator the reader could not distinguish from a blank line the prose actually wanted. The
last one is why blocks now close with an explicit `@end` rather than a blank line: a
terminator that means "end" cannot be confused with content that happens to be empty.

Also ported: `shaders` and `mine`. Four Python scripts remain, none of them gated - the build
and the gate are Python-free.

