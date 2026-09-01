# D253 - The framebuffer alignment, which was the answer all along


*status: measured*

```text
onion,0x4000,1     0x80290015
garlic,0x4000,1    0x80290015
garlic,0x10000,1   0x0          accepted
```

**`0x4000` is not a coarse enough alignment for a scanout buffer; `0x10000` is.** That is the
entire defect. The display now comes up:

```text
OBS|display|ready|1920x1080 framebuffer|0x0
OBS|display|presenting|a submitted frame reached the display|0x0
```

`presenting` is measured rather than assumed - the frame counter moved, which is a stronger
statement than the flip being accepted (D187).

**Why it took so long is the part worth keeping.** `0x80290015` is returned identically whatever
else is right or wrong, so every attempt that varied the attribute got the same number back and
read it as "still wrong somewhere". The alignment was never varied, because it was not a
*display* parameter - it was an argument to `sceKernelAllocateDirectMemory` three steps earlier,
and the search had been framed as "which video argument is wrong".

The sweep found it in one run by varying things on the *other* side of the call, and it only
did that because the attribute sweep had first proved the attribute was not at fault. Neither
half would have been enough alone.

Memory type turned out not to matter: onion and garlic both refuse at `0x4000`. Garlic is what
the accepted combination used, so garlic is what ships - the measured combination, not a
reasoned one.

