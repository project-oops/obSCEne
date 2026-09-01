# It draws its own report


obSCEne now renders its results to the screen: a title, live progress, one row per
section with a proportion bar and counts, and the totals. Redrawn after every section,
so a run that dies partway leaves its progress visible (D041).

**Built in the order the repo insists on.** The font was generated from glyph art and
rendered to a PNG to be checked by eye before anything used it. The renderer was then
run under `make host`, which draws through the identical code and writes a PNG - so
every pixel was verified without an emulator, a module format, or a loader in the way.
Only then did it go near a console target.

**Two real problems, both found by running it.**

*The framebuffer length was refused.* 1920x1080x4 is `0x7E9000`, which is not a whole
number of 16 KiB pages, and the refusal says only "no direct memory" - which reads as
"not enough" rather than "not a multiple of the granularity". Rounded up.

*The video checks were closing the display's output.* `080-video/open` opens the main
output and hands it straight back; with the display live that tears the registration
down underneath it. Every subsequent flip was refused and the emulator faulted inside
its own presenter thread on the second one. The run dropped from 511 records to 155.
Both video checks now yield to the display, and a refused flip disables flipping rather
than repeating (D042).

**Where it ended up**

    OBS|display|ready|1920x1080 framebuffer
    511 records, 0 refused flips, 0 presenter faults
    53 pass  2 partial  18 fail  6 skip

The two remaining skips beyond the excluded pair are the video checks standing aside for
the screen, which is visible in the report and in the drawn output as an all-skip row.

