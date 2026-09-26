# D041 - The report is drawn as well as written

**Status:** decided
**Date:** 2026-09-26

Where a display opens, the probe draws its report to the framebuffer, redrawn at each section
boundary: a summary of sections, then pages of checks. The text stream stays the contract, and an
`OBS|display` record says whether the screen can be believed.

**Why:** a loader with no working text channel, or a person watching a black window, otherwise
learns nothing. Redrawing per section leaves a dead run showing how far it got.

**Rejected:** text only - silent on loaders whose output channels fail. Redrawing per check -
a full present per check perturbs the timing it measures.
