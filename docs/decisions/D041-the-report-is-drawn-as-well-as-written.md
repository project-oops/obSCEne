# D041 - The report is drawn as well as written


Status: decided.

The text stream stays the contract - it is what `verify` and `diff` read. But it has two
failure modes that leave a run saying nothing: an emulator with no working write path
discards every byte, and a person watching a run sees a black window and cannot tell a
working probe from a hung one. Both happened during this work.

A framebuffer answers both, and it needs no output function to work at all. Text for
machines, pixels for people; either surviving alone beats the current all-or-nothing.

**Redrawn after every section**, not once at the end, so a run that dies partway leaves
the screen showing how far it got. That is the thing a black window cannot say.

**It shows sections, not checks.** Seventy-nine identifiers at a readable size does not
fit, and a screen that has to be squinted at has lost its only advantage over the text.
The stream keeps individual verdicts; the screen carries the shape of the run.

**The stream says whether the screen can be believed.** `OBS|display|<state>|<detail>` is
emitted before any check. A photograph of a screen cannot distinguish "these are the
results" from "the display never came up and this is a stale frame"; that record can.

**Verified on the host before it was trusted anywhere.** The host build draws through the
identical clear, rect and text code and writes a PNG, so a glyph that is upside down is
obvious in a file rather than invisible in a black window (D001). The font was checked
the same way before a single pixel reached an emulator.

