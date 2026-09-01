# D147 - The three letters in the middle of the name are marked wherever the project renders it


`obSCEne` has always been built around a piece of the platform's name, and until now that
was a spelling nobody could see. It is now drawn apart: magenta on the screen
(`OBS_COLOUR_MARK`), magenta in `obscene-tool pretty`, bold in the README heading.

This sits with principle 5 rather than against it. The rule is no vendor trademarks *in
prose* - "the target platform", "the vendor" - and marking three letters of this project's
own name spells nothing out. It points at the joke the name already makes.

**No new mechanism was needed anywhere.** `obs_display_text` already returned the x to
continue at, with a header comment saying it was so callers could chain runs of different
colours - written for this and unused until now. `pretty`'s palette already had an
`emphasise` helper and a status colour table.

Three details worth keeping:

- **The colour matches no status.** Mid-saturation like `PASS`, `PARTIAL`, `FAIL` and
  `ACCENT` so the banner reads as one palette, and deliberately not equal to any of them,
  because the mark grades nothing.
- **The terminal run ends with `39` (default foreground), not `RESET`.** The banner is
  already inside a bold run, and `RESET` clears bold along with colour - the rest of the
  line would have come out unemphasised, which is the kind of thing that looks like a
  terminal quirk rather than a bug.
- **The wordmark is a function, so the width is measured rather than assumed.** Two call
  sites carried `8 * 6 * 7` - font width times scale times letter count - to work out where
  to put the HUD next to it. `obs_draw_wordmark` returns where it actually finished, so
  changing the scale or the name can no longer leave the HUD drawn over the title.

**Not applied to prose.** The name appears some 160 times across the docs and bolding every
one would be noise rather than identity - the mark belongs on the wordmark, which is what a
heading and a banner are, and not on every sentence that happens to mention the program.

