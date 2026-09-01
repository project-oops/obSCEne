# 2026-08-30 - obSCEne drew its own report on the console's display


```text
OBS|display|ready|1920x1080 framebuffer|0x0
OBS|display|presenting|a submitted frame reached the display|0x0
```

**Evidence:** the records above, and
[`docs/screenshots/ps5-hardware.png`](../screenshots/ps5-hardware.png) - a screen legible enough
to read every check name.
**Build:** `DISPLAY_MEM=0` (write-back onion), framebuffer aligned to `0x10000`, two buffers.

`presenting` is measured rather than assumed - the frame counter moved, which is a stronger
statement than the flip having been accepted (D187).

**The defect was an alignment**, three steps earlier than anyone was looking. `0x4000` is
refused for a scanout buffer and `0x10000` is accepted, and `0x80290015` is returned
identically whatever else is right or wrong - so five sessions of varying *display* arguments
never moved it. It was an argument to `sceKernelAllocateDirectMemory`. (D253)

Then the picture tore, because one buffer means drawing into the memory being scanned out.
Two buffers and a bounded wait for the flip to take effect fixed it. Not an SDK problem - an
ordinary choice about how many buffers to allocate, made the wrong way. (D256, D257)

---

