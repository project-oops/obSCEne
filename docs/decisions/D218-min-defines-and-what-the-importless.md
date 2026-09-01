# D218 - `MIN_DEFINES`, and what the importless control turned out not to prove


Status: **decided**, 2026-08-29.

`eboot-min` grew a hook for the variants already in `src/min.c`, because once an eboot *loads*
the question changes: a fault inside `libkernel` is either the platform library's own
initialisation or our first call into it, and those need opposite fixes.

**The importless build cannot answer it.** It was meant to be the control - loads `libkernel`,
carries the vendor segment and the process parameters, calls nothing - but it does not load at
all, and the loader says why:

```text
[rtld] ERROR preprocess_dt_entries:9632: C: ah 1  pg 0  jr 0  pr 1  prs 0  rl 0
[rtld] ERROR preprocess_dt_entries:9636: C: rls 0  rle 0  stt 1 sts 1  st 1  se 1
```

With no imports there are no relocations, so `PLTGOT`, `JMPREL`, `PLTRELSZ`, `RELA`, `RELASZ`
and `RELAENT` are absent - and this loader requires the full set unconditionally, empty or not.
That is a fact about the format worth having, and it retires the control.

The file-writing variant answered it instead, and negatively. Built with three imports rather
than one and a different code path, it faults at the **same instruction and the same address**
as the one-import build - `rip 0x80003333b`, writing to `0x28` - and leaves no file. Identical
failure from different code means our code is not running: the fault is in `libkernel`'s
initialisation, before our entry point.

The next thing to look at is the process parameter block, which is what `libkernel` reads before
a single instruction of ours executes. Ours is `0x50` bytes and zero after the magic; a real
one is `0x60`, declares an SDK version of `0x08008011` and an entry count of five, and carries
three non-null pointers at `+0x38`, `+0x40` and `+0x48` into its RELRO segment. A write to
`null + 0x28` is what taking one of those pointers and finding zero looks like.

