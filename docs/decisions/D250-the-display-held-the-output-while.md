# D250 - The display held the output while answering that it did not


*status: measured*

`obs_display_holds_output()` returned `obs_state == OBS_DISPLAY_READY`. The display opened the
output, failed three steps later at the framebuffer, and **never closed the handle** - so it was
holding the console's main output while answering "no" to the one question about that.

`080-video/open` then opened the same output, got `0x80290009`, and reported *"the main video
output would not open"* with `OBS_FROM_ASSUMED` provenance. **A finding this program invented
about the console, from its own leak.** It sat in `docs/HARDWARE.md` as a hardware fact.

Two changes. Every give-up releases the output, because a give-up is a decision to stop and
stopping while holding the main display is never right. And `holds_output` is written against
the handle rather than against the state, because those are different questions and the caller
is asking the one about the handle.

With the leak gone, `080-video/open` passes: `0x4e100200`. The output opens fine and always did.

**The framebuffer is deliberately still not released** - handing back memory the display may be
scanning out of is worse than leaking it at exit. An output whose buffers were refused has
nothing scanning out of it, which is why the handle is different.

